// @pvz-online: Exercise live connections and wire accounting so cached status
// cannot silently report stale room membership or hide failed writes.
package main

import (
	"bytes"
	"encoding/json"
	"io"
	"net"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
	"time"
)

func TestStatusLoopbackOnly(t *testing.T) {
	for _, address := range []string{"0.0.0.0:27778", ":27778", "192.168.1.1:27778", "localhost:27778", "[::]:27778", "bad"} {
		if localAdminAddress(address) == nil {
			t.Errorf("accepted non-loopback IP address %q", address)
		}
	}
	for _, address := range []string{"127.0.0.1:27778", "[::1]:27778"} {
		if err := localAdminAddress(address); err != nil {
			t.Error(err)
		}
	}
	server := newServer(t.Logf)
	if _, err := server.startStatus("0.0.0.0:0"); err == nil {
		t.Fatal("public status listener accepted")
	}
}

func TestStatusLiveRoomsPlayersAndPrivacy(t *testing.T) {
	s, addr := startTestServer(t, nil)
	host := newTestClient(t, addr)
	host.send(msgCliCreateRoom, createPayload(1, 68, "DoNotExposeName"))
	w := decodeWelcome(t, host.next(time.Second).payload)
	host.next(time.Second)
	publishRoom(t, host, true, false)
	guest := newTestClient(t, addr)
	guest.send(msgCliDirectoryJoin, joinPayload(1, 68, "SecretGuest", w.code))
	guest.next(time.Second)
	guest.next(time.Second)
	host.next(time.Second)
	anonymous := newTestClient(t, addr)
	deadline := time.Now().Add(time.Second)
	for s.stats.accepted.Load() < 3 && time.Now().Before(deadline) {
		time.Sleep(time.Millisecond)
	}
	sample := s.sampleStatus(time.Now())
	if sample.Connections != 3 || sample.Players != 2 || sample.AnonymousConnections != 1 || sample.Rooms != 1 || sample.PublicWaitingRooms != 1 {
		t.Fatalf("wrong live counts: %+v", sample)
	}
	encoded, _ := json.Marshal(sample)
	for _, secret := range []string{"DoNotExposeName", "SecretGuest", w.code} {
		if strings.Contains(string(encoded), secret) {
			t.Fatalf("status leaks %q", secret)
		}
	}
	publishRoom(t, host, true, true)
	guest.nc.Close()
	anonymous.nc.Close()
	deadline = time.Now().Add(time.Second)
	for time.Now().Before(deadline) {
		s.mu.Lock()
		held := s.rooms[w.code].holdActive(2, time.Now())
		s.mu.Unlock()
		if held && s.stats.closed.Load() >= 2 {
			break
		}
		time.Sleep(time.Millisecond)
	}
	next := s.sampleStatus(time.Now().Add(2 * time.Second))
	if next.StartedRooms != 1 || next.PublicWaitingRooms != 0 || next.Players != 1 || next.HeldSeats != 1 || next.Connections != 1 {
		t.Fatalf("wrong disconnected/start counts: %+v", next)
	}
}

func TestStatusWireCountersAndDirectory(t *testing.T) {
	s, addr := startTestServer(t, nil)
	c := newTestClient(t, addr)
	c.send(msgCliList, []byte{1, 0, 68, 0, 0, 0})
	frame := c.next(time.Second)
	if frame.typ != msgSrvDirectory {
		t.Fatal(frame.typ)
	}
	c.expectClosed(time.Second)
	deadline := time.Now().Add(time.Second)
	for s.stats.closed.Load() != 1 && time.Now().Before(deadline) {
		time.Sleep(time.Millisecond)
	}
	sample := s.sampleStatus(time.Now())
	if sample.AcceptedTotal != 1 || sample.ClosedTotal != 1 || sample.DirectoryQueriesTotal != 1 || sample.ReceivedBytesTotal != 10 || sample.SentBytesTotal != 12 || sample.ReceivedFramesTotal != 1 || sample.SentFramesTotal != 1 {
		t.Fatalf("wire counters: %+v", sample)
	}
}

func TestStatusRatesAreCachedAndMonotonic(t *testing.T) {
	s := newServer(t.Logf)
	now := time.Now()
	s.sampleStatus(now)
	s.stats.inBytes.Add(200)
	s.stats.outBytes.Add(400)
	s.stats.forwarded.Add(20)
	s.stats.directoryQueries.Add(4)
	cached := s.sampleStatus(now.Add(100 * time.Millisecond))
	if cached.SentBytesTotal != 0 {
		t.Fatal("sample cache bypassed")
	}
	v := s.sampleStatus(now.Add(2 * time.Second))
	if v.ReceivedBytesPerSecond != 100 || v.SentBytesPerSecond != 200 || v.ForwardedFramesPerSecond != 10 || v.DirectoryQueriesPerSecond != 2 {
		t.Fatalf("rates: %+v", v)
	}
	if v.CPUPercentOneCore != nil && *v.CPUPercentOneCore < 0 {
		t.Fatal("negative CPU")
	}
}

func TestStatusHTTPCLIAndMethods(t *testing.T) {
	s := newServer(t.Logf)
	server := httptest.NewServer(s.statusHandler())
	defer server.Close()
	for _, path := range []string{"/", "/healthz", "/status"} {
		response, err := http.Get(server.URL + path)
		if err != nil {
			t.Fatal(err)
		}
		data, _ := io.ReadAll(response.Body)
		response.Body.Close()
		if response.StatusCode != 200 || len(data) == 0 || response.Header.Get("Cache-Control") != "no-store" {
			t.Fatalf("bad %s", path)
		}
	}
	response, err := http.Post(server.URL+"/status", "text/plain", nil)
	if err != nil {
		t.Fatal(err)
	}
	response.Body.Close()
	if response.StatusCode != 405 {
		t.Fatal("status permits writes")
	}
	response, err = http.Get(server.URL + "/rooms")
	if err != nil {
		t.Fatal(err)
	}
	response.Body.Close()
	if response.StatusCode != 404 {
		t.Fatal("room details exposed")
	}
	var plain, raw bytes.Buffer
	address := strings.TrimPrefix(server.URL, "http://")
	if err := printStatus(address, false, &plain); err != nil {
		t.Fatal(err)
	}
	if !strings.Contains(plain.String(), "连接 0/512") || !strings.Contains(plain.String(), relayRelease) {
		t.Fatal(plain.String())
	}
	if err := printStatus(address, true, &raw); err != nil {
		t.Fatal(err)
	}
	var v relayStatus
	if err := json.Unmarshal(raw.Bytes(), &v); err != nil || v.Release != relayRelease {
		t.Fatal(raw.String())
	}
}

func TestStatusQueueOverflowAndCloseExactlyOnce(t *testing.T) {
	s := newServer(t.Logf)
	a, b := net.Pipe()
	defer b.Close()
	c := newConn(s, a)
	for i := 0; i < 64; i++ {
		if !s.enqueue(c, []byte{1}) {
			t.Fatal("early queue overflow")
		}
	}
	if s.enqueue(c, []byte{1}) {
		t.Fatal("overflow accepted")
	}
	c.close()
	v := s.sampleStatus(time.Now())
	if v.QueueDropsTotal != 1 || v.ClosedTotal != 1 || v.Connections != 0 {
		t.Fatalf("queue/close: %+v", v)
	}
}

type partialWriteConn struct{ net.Conn }

func (c partialWriteConn) Write(p []byte) (int, error) { return len(p) - 1, nil }
func TestStatusPartialWriteIsNotCountedAsComplete(t *testing.T) {
	s := newServer(t.Logf)
	a, b := net.Pipe()
	defer b.Close()
	c := newConn(s, partialWriteConn{a})
	if c.write([]byte{1, 2, 3, 4}) {
		t.Fatal("partial write accepted")
	}
	v := s.sampleStatus(time.Now())
	if v.SentBytesTotal != 3 || v.SentFramesTotal != 0 || v.WriteErrorsTotal != 1 || v.Connections != 0 {
		t.Fatalf("partial write counters: %+v", v)
	}
}

func TestStatusMalformedAndControlLimitCounters(t *testing.T) {
	s := newServer(t.Logf)
	a, b := net.Pipe()
	defer b.Close()
	c := newConn(s, a)
	for i := 0; i < 33; i++ {
		s.handle(c, msgCliPong, nil)
	}
	a, b = net.Pipe()
	defer b.Close()
	badGame := newConn(s, a)
	s.handle(badGame, 4, []byte{1})
	a, b = net.Pipe()
	defer b.Close()
	badControl := newConn(s, a)
	s.handle(badControl, msgCliPong, []byte{1})
	v := s.sampleStatus(time.Now())
	if v.RateLimitedTotal != 1 || v.MalformedTotal != 2 || v.ClosedTotal != 3 || v.Connections != 0 {
		t.Fatalf("rejection counters: %+v", v)
	}
}
