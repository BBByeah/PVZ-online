package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"testing"
	"time"
)

func lookupPayload(code string) []byte { p := []byte{3, 0, 90, 0}; return append(p, code...) }
func lookupTCP(t *testing.T, addr, code string) []byte {
	t.Helper()
	q := newTestClient(t, addr)
	q.send(msgCliList, lookupPayload(code))
	f := q.next(time.Second)
	if f.typ != msgSrvDirectory || len(f.payload) != 11 || binary.LittleEndian.Uint16(f.payload) != 3 || string(f.payload[3:7]) != code {
		t.Fatalf("invalid v3 PAGE: %#v", f)
	}
	return f.payload
}

func TestLookupV3MalformedRequestsCloseWithoutFalseAbsent(t *testing.T) {
	for _, p := range [][]byte{{3, 0}, {3, 0, 90, 0, 'A', 'B'}, {3, 0, 90, 0, 'A', 'B', 'C', 'D', 0}, {3, 0, 90, 0, 'a', 'b', 'c', 'd'}, {3, 0, 90, 0, '0', '0', '0', '0'}} {
		s, addr := startTestServer(t, nil)
		q := newTestClient(t, addr)
		q.send(msgCliList, p)
		q.nc.SetReadDeadline(time.Now().Add(time.Second))
		_, _, err := readFrame(q.nc)
		if err == nil {
			t.Fatalf("malformed request got success/absent: %x", p)
		}
		if s.stats.malformed.Load() != 1 {
			t.Fatalf("bad request not classified: %x", p)
		}
	}
}

func TestLookupV3RateLimitedTCPDoesNotAnswerAbsent(t *testing.T) {
	s, addr := startTestServer(t, nil)
	s.mu.Lock()
	s.lookupWindow = time.Now()
	s.lookupCount = 128
	s.mu.Unlock()
	q := newTestClient(t, addr)
	q.send(msgCliList, lookupPayload("ABCD"))
	q.nc.SetReadDeadline(time.Now().Add(time.Second))
	_, p, err := readFrame(q.nc)
	if err == nil || bytes.Equal(p, []byte{3, 0, 0, 'A', 'B', 'C', 'D', 0, 0, 0, 0}) {
		t.Fatal("limited lookup masqueraded as absence")
	}
}
func renameLookupRoom(t *testing.T, s *Server, old, code string) {
	t.Helper()
	s.mu.Lock()
	defer s.mu.Unlock()
	r := s.rooms[old]
	delete(s.rooms, old)
	r.code = code
	s.rooms[code] = r
}
func TestLookupV3TwoTCPNodesFindHiddenRoomWithoutSeat(t *testing.T) {
	s1, a1 := startTestServer(t, nil)
	s2, a2 := startTestServer(t, nil)
	h1 := newTestClient(t, a1)
	h2 := newTestClient(t, a2)
	old1 := decodeWelcome(t, h1.create(1, 90, "host-one")).code
	old2 := decodeWelcome(t, h2.create(1, 90, "host-two")).code
	renameLookupRoom(t, s1, old1, "WXYZ")
	renameLookupRoom(t, s2, old2, "ABCD")
	if got := lookupTCP(t, a1, "ABCD"); got[2] != 0 || got[7] != 0 || got[8] != 0 || got[9] != 0 || got[10] != 0 {
		t.Fatalf("absent metadata not zero: %x", got)
	}
	if got := lookupTCP(t, a2, "ABCD"); got[2] != 1 || binary.LittleEndian.Uint16(got[7:]) != 90 || got[9] != 1 || got[10] != 1 {
		t.Fatalf("hidden room mismatch: %x", got)
	}
	for _, s := range []*Server{s1, s2} {
		s.mu.Lock()
		for _, r := range s.rooms {
			if r.connectedPlayers(true) != 1 {
				t.Error("lookup occupied a player seat")
			}
		}
		s.mu.Unlock()
	}
	// Only join once, to the confirmed node; never probe by actual JOIN.
	g := newTestClient(t, a2)
	w := decodeWelcome(t, g.join(1, 90, "join-after-lookup", "ABCD"))
	if w.yourSeat != 2 {
		t.Fatal("confirmed node join failed")
	}
}
func TestLookupV3DuplicateAndStartedFlags(t *testing.T) {
	s1, a1 := startTestServer(t, nil)
	s2, a2 := startTestServer(t, nil)
	for i, v := range []struct {
		s    *Server
		addr string
	}{{s1, a1}, {s2, a2}} {
		h := newTestClient(t, v.addr)
		old := decodeWelcome(t, h.create(1, 90, fmt.Sprintf("host%d", i))).code
		renameLookupRoom(t, v.s, old, "ABCD")
		v.s.mu.Lock()
		r := v.s.rooms["ABCD"]
		r.started = true
		r.joinOpen = false
		v.s.mu.Unlock()
	}
	for _, a := range []string{a1, a2} {
		got := lookupTCP(t, a, "ABCD")
		if got[2] != 1 || got[10] != 7 {
			t.Fatalf("duplicate must remain visible, flags=%x", got)
		}
	}
	// Two hits remain two; relay must not disambiguate by public status/order.
}
func TestLookupV3FullAndHeldHost(t *testing.T) {
	s, addr, code, c := sixRoom(t)
	got := lookupTCP(t, addr, code)
	if got[2] != 1 || got[9] != 6 || got[10]&8 == 0 {
		t.Fatalf("full metadata=%x", got)
	}
	c[0].nc.Close()
	flowWait(t, s, code, func(r *Room) bool { return r.bySeat[1] == nil && r.holdActive(1, time.Now()) })
	got = lookupTCP(t, addr, code)
	if got[2] != 1 || got[9] != 6 {
		t.Fatalf("held metadata=%x", got)
	}
}
func TestLookupV3RateAndMemoryBounds(t *testing.T) {
	s := newServer(nil)
	now := time.Unix(100, 0)
	for i := 0; i < 8; i++ {
		if !s.allowRoomLookup("192.0.2.1:123", now) {
			t.Fatal("initial budget lost")
		}
	}
	if s.allowRoomLookup("192.0.2.1:456", now) {
		t.Fatal("port rotation bypassed IP bound")
	}
	if !s.allowRoomLookup("192.0.2.1:456", now.Add(time.Second)) {
		t.Fatal("budget did not recover")
	}
	for i := 0; i < 2200; i++ {
		s.allowRoomLookup(fmt.Sprintf("198.51.%d.%d:1", i/256, i%256), now.Add(time.Duration(i+2)*time.Second))
	}
	if len(s.lookupRates) > 2048 {
		t.Fatal("lookup rate map grew without bound")
	}
}
