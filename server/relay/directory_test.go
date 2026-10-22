package main

import (
	"encoding/binary"
	"fmt"
	"testing"
	"time"
)

func publishRoom(t *testing.T, c *testClient, public, started bool) {
	t.Helper()
	p := []byte{1, 0, 0, 0}
	if public {
		p[2] = 1
	}
	if started {
		p[3] = 1
	}
	c.send(msgCliPublish, p)
	f := c.next(time.Second)
	if f.typ != msgSrvPublished || string(f.payload) != string(p) {
		t.Fatalf("publish ACK: %#v", f)
	}
}
func directoryPage(t *testing.T, addr string, build, offset uint16) ([]byte, uint16) {
	t.Helper()
	return directoryVersionPage(t, addr, 1, build, offset)
}
func directoryVersionPage(t *testing.T, addr string, version, build, offset uint16) ([]byte, uint16) {
	t.Helper()
	c := newTestClient(t, addr)
	p := make([]byte, 6)
	binary.LittleEndian.PutUint16(p, version)
	binary.LittleEndian.PutUint16(p[2:], build)
	binary.LittleEndian.PutUint16(p[4:], offset)
	c.send(msgCliList, p)
	f := c.next(time.Second)
	if f.typ != msgSrvDirectory || len(f.payload) < 8 || len(f.payload) != 8+int(f.payload[7])*24 || len(f.payload) > maxPayload {
		t.Fatalf("directory page %#v", f)
	}
	if binary.LittleEndian.Uint16(f.payload) != version || binary.LittleEndian.Uint16(f.payload[3:]) != offset {
		t.Fatal("response identity")
	}
	c.expectClosed(time.Second)
	return f.payload[8:], binary.LittleEndian.Uint16(f.payload[5:])
}
func newDirectoryRoom(t *testing.T, addr string, name string) (*testClient, string) {
	t.Helper()
	c := newTestClient(t, addr)
	w := decodeWelcome(t, c.create(1, 68, name))
	admissionCredential(t, c)
	return c, w.code
}
func TestDirectoryVisibilityAndStarted(t *testing.T) {
	_, addr := startTestServer(t, nil)
	host, code := newDirectoryRoom(t, addr, "public")
	if p, _ := directoryPage(t, addr, 68, 0); len(p) != 0 {
		t.Fatal("unpublished room leaked")
	}
	publishRoom(t, host, true, false)
	if p, _ := directoryPage(t, addr, 68, 0); len(p) != 24 || string(p[:4]) != code || p[22] != 1 || p[23] != 0 {
		t.Fatalf("public room %x", p)
	}
	publishRoom(t, host, false, false)
	if p, _ := directoryPage(t, addr, 68, 0); len(p) != 0 {
		t.Fatal("private room leaked")
	}
	publishRoom(t, host, true, true)
	if p, _ := directoryPage(t, addr, 68, 0); len(p) != 0 {
		t.Fatal("running room leaked")
	}
}
func TestDirectoryStaleJoinAndCodeAdmission(t *testing.T) {
	_, addr := startTestServer(t, nil)
	host, code := newDirectoryRoom(t, addr, "stale")
	publishRoom(t, host, true, false)
	directoryPage(t, addr, 68, 0)
	publishRoom(t, host, true, true)
	late := newTestClient(t, addr)
	late.send(msgCliDirectoryJoin, joinPayload(1, 68, "late", code))
	late.expectReject(rejectJoinClosed)
	// Endless preparation can open the code gate while remaining absent from the public list.
	host.send(msgCliGate, admissionGate(1, 2, true))
	gateAck(t, host, 1)
	byCode := newTestClient(t, addr)
	byCode.join(1, 68, "code", code)
	admissionCredential(t, byCode)
	if p, _ := directoryPage(t, addr, 68, 0); len(p) != 0 {
		t.Fatal("intermission leaked into directory")
	}
}
func TestDirectoryPrivateJoinAndHostAuthority(t *testing.T) {
	_, addr := startTestServer(t, nil)
	host, code := newDirectoryRoom(t, addr, "private")
	publishRoom(t, host, false, false)
	guest := newTestClient(t, addr)
	guest.send(msgCliDirectoryJoin, joinPayload(1, 68, "guest", code))
	guest.expectReject(rejectJoinClosed)
	guest.join(1, 68, "guest", code)
	admissionCredential(t, guest)
	host.next(time.Second)
	guest.send(msgCliPublish, []byte{1, 0, 1, 0})
	guest.expectNothing(25 * time.Millisecond)
	if p, _ := directoryPage(t, addr, 68, 0); len(p) != 0 {
		t.Fatal("guest changed privacy")
	}
}
func TestDirectoryPaginationVersionAndFull(t *testing.T) {
	_, addr := startTestServer(t, nil)
	var hosts []*testClient
	var codes []string
	for i := 0; i < 8; i++ {
		h, code := newDirectoryRoom(t, addr, fmt.Sprintf("host%d", i))
		hosts = append(hosts, h)
		codes = append(codes, code)
		publishRoom(t, h, true, false)
	}
	p, next := directoryPage(t, addr, 68, 0)
	if len(p) != 6*24 || next != 6 {
		t.Fatalf("first page len=%d next=%d", len(p), next)
	}
	p2, last := directoryPage(t, addr, 68, 6)
	if len(p2) != 2*24 || last != 65535 {
		t.Fatal("last page")
	}
	seen := map[string]bool{}
	for _, page := range [][]byte{p, p2} {
		for i := 0; i < len(page); i += 24 {
			code := string(page[i : i+4])
			if seen[code] {
				t.Fatal("duplicate room")
			}
			seen[code] = true
		}
	}
	old, _ := directoryPage(t, addr, 67, 0)
	for i := 23; i < len(old); i += 24 {
		if old[i]&2 == 0 {
			t.Fatal("missing build mismatch")
		}
	}
	for i := 0; i < 5; i++ {
		guest := newTestClient(t, addr)
		guest.join(1, 68, fmt.Sprintf("p%d", i), codes[0])
		admissionCredential(t, guest)
		hosts[0].next(time.Second)
	}
	full, _ := directoryPage(t, addr, 68, 0)
	if !seen[codes[0]] {
		t.Fatal("fixture")
	}
	if len(full) == 0 {
		t.Fatal("full directory empty")
	}
	all, _ := directoryPage(t, addr, 68, 6)
	full = append(full, all...)
	found := false
	for i := 0; i < len(full); i += 24 {
		if string(full[i:i+4]) == codes[0] {
			found = true
			if full[i+22] != 6 || full[i+23]&1 == 0 {
				t.Fatal("full flag")
			}
		}
	}
	if !found {
		t.Fatal("full room hidden")
	}
	extra := newTestClient(t, addr)
	extra.send(msgCliDirectoryJoin, joinPayload(1, 68, "extra", codes[0]))
	extra.expectReject(rejectRoomFull)
}
func TestDirectoryForwardStartClosesListing(t *testing.T) {
	_, addr := startTestServer(t, nil)
	host, code := newDirectoryRoom(t, addr, "start")
	publishRoom(t, host, true, false)
	guest := newTestClient(t, addr)
	guest.join(1, 68, "guest", code)
	admissionCredential(t, guest)
	host.next(time.Second)
	host.send(3, []byte{1, 2, 0, 0})
	if guest.next(time.Second).typ != 3 {
		t.Fatal("start not forwarded")
	}
	if p, _ := directoryPage(t, addr, 68, 0); len(p) != 0 {
		t.Fatal("start did not withdraw room")
	}
}
func TestDirectoryHostOfflineAndClosedGate(t *testing.T) {
	s, addr := startTestServer(t, nil)
	host, code := newDirectoryRoom(t, addr, "offline")
	publishRoom(t, host, true, false)
	host.send(msgCliGate, admissionGate(1, 1, false))
	gateAck(t, host, 1)
	if p, _ := directoryPage(t, addr, 68, 0); len(p) != 0 {
		t.Fatal("closed gate listed")
	}
	host.send(msgCliGate, admissionGate(2, 1, true))
	gateAck(t, host, 2)
	host.nc.Close()
	deadline := time.Now().Add(time.Second)
	for time.Now().Before(deadline) {
		s.mu.Lock()
		offline := s.rooms[code].host == nil
		s.mu.Unlock()
		if offline {
			break
		}
		time.Sleep(time.Millisecond)
	}
	if p, _ := directoryPage(t, addr, 68, 0); len(p) != 0 {
		t.Fatal("offline host listed")
	}
}
func TestDirectoryMalformedRequests(t *testing.T) {
	_, addr := startTestServer(t, nil)
	for _, p := range [][]byte{{}, {1, 0, 68, 0, 1, 0}, {1, 0, 68, 0, 128, 0}, {1, 0, 68, 0, 132, 0}, {1, 0, 68, 0, 0, 0, 0}, {2, 0, 68, 0, 2, 1}, {2, 0, 68, 0, 255, 0}, {3, 0, 68, 0, 0, 0}} {
		c := newTestClient(t, addr)
		c.send(msgCliList, p)
		c.expectClosed(time.Second)
	}
}

func TestRooms256CapacityDirectoryCompatibilityAndReuse(t *testing.T) {
	s, addr := startTestServer(t, nil)
	hosts := make([]*testClient, 0, 256)
	codes := make(map[string]bool)
	for i := 0; i < 256; i++ {
		h, code := newDirectoryRoom(t, addr, fmt.Sprintf("host%d", i))
		hosts = append(hosts, h)
		codes[code] = true
		publishRoom(t, h, true, false)
	}
	if len(codes) != 256 {
		t.Fatal("room codes were reused")
	}
	extra := newTestClient(t, addr)
	extra.send(msgCliCreateRoom, createPayload(1, 68, "overflow"))
	extra.expectReject(rejectServerBusy)
	for _, fixture := range []struct{ version, want uint16 }{{1, 128}, {2, 256}} {
		seen := make(map[string]bool)
		for offset := uint16(0); ; {
			p, next := directoryVersionPage(t, addr, fixture.version, 68, offset)
			for i := 0; i < len(p); i += 24 {
				code := string(p[i : i+4])
				if !codes[code] || seen[code] {
					t.Fatalf("version %d invalid/repeated code %q", fixture.version, code)
				}
				seen[code] = true
			}
			if next == 65535 {
				break
			}
			if next != offset+directoryPageSize || next >= fixture.want {
				t.Fatalf("version %d unexpected next page %d", fixture.version, next)
			}
			offset = next
		}
		if len(seen) != int(fixture.want) {
			t.Fatalf("version %d enumerated %d rooms, want %d", fixture.version, len(seen), fixture.want)
		}
	}
	// Rooms beyond the old directory window still accept their original code.
	last, next := directoryVersionPage(t, addr, 2, 68, 252)
	if len(last) != 4*24 || next != 65535 {
		t.Fatal("256-room final page")
	}
	guest := newTestClient(t, addr)
	guest.join(1, 68, "last-room", string(last[:4]))
	admissionCredential(t, guest)
	status := s.sampleStatus(time.Now())
	if status.Rooms != 256 || status.RoomLimit != 256 || status.ConnectionLimit != 512 {
		t.Fatalf("capacity status: rooms=%d limit=%d connections=%d", status.Rooms, status.RoomLimit, status.ConnectionLimit)
	}
	hosts[0].send(msgCliLeaveRoom, nil)
	deadline := time.Now().Add(time.Second)
	for {
		s.mu.Lock()
		count := len(s.rooms)
		s.mu.Unlock()
		if count == 255 {
			break
		}
		if time.Now().After(deadline) {
			t.Fatal("closed room did not release capacity")
		}
		time.Sleep(time.Millisecond)
	}
	newDirectoryRoom(t, addr, "reused")
}
