package main

import (
	"encoding/binary"
	"math"
	"strings"
	"testing"
	"time"
)

func TestBoundedNodeInfoRates(t *testing.T) {
	for _, item := range []struct {
		value float64
		want  uint32
	}{{-1, 0}, {math.NaN(), 0}, {1000, 1000}, {1e12, math.MaxUint32}, {math.Inf(1), math.MaxUint32}} {
		if got := boundedRate(item.value); got != item.want {
			t.Fatalf("rate %v: %d != %d", item.value, got, item.want)
		}
	}
}

func TestNodeInfoBoundedAndIndependent(t *testing.T) {
	_, addr := startTestServer(t, nil)
	host, code := newDirectoryRoom(t, addr, "private-host")
	query := newTestClient(t, addr)
	query.send(msgCliNodeInfo, []byte{1, 0})
	f := query.next(time.Second)
	if f.typ != msgSrvNodeInfo || len(f.payload) != 16 {
		t.Fatalf("node info shape: %#v", f)
	}
	u16 := func(at int) uint16 { return binary.LittleEndian.Uint16(f.payload[at:]) }
	if u16(0) != 1 || u16(2) != protocolVersion || u16(4) != 1 || u16(6) != 256 || u16(8) != 1 || u16(12) != 512 || u16(14) != 3 {
		t.Fatalf("node capacity: %x", f.payload)
	}
	query.expectClosed(time.Second)
	host.send(msgCliNodeInfo, []byte{1, 0})
	publishRoom(t, host, false, false)
	// Query must not consume a seat, publish a room, or replace its connection.
	if p, _ := directoryPage(t, addr, 68, 0); len(p) != 0 {
		t.Fatal("private room published by metadata query")
	}
	join := newTestClient(t, addr)
	w := decodeWelcome(t, join.join(1, 68, "joiner", code))
	if w.yourSeat != 2 {
		t.Fatal("metadata query consumed a seat")
	}
}

func TestIndependentNodesMayHaveSameRoomCode(t *testing.T) {
	type node struct {
		server        *Server
		address, name string
	}
	s1, a1 := startTestServer(t, nil)
	s2, a2 := startTestServer(t, nil)
	for _, n := range []node{{s1, a1, "Node1Host"}, {s2, a2, "Node2Host"}} {
		_, oldCode := newDirectoryRoom(t, n.address, n.name)
		n.server.mu.Lock()
		r := n.server.rooms[oldCode]
		delete(n.server.rooms, oldCode)
		r.code = "ABCD"
		n.server.rooms[r.code] = r
		n.server.mu.Unlock()
	}
	for _, n := range []node{{s1, a1, "Node1Host"}, {s2, a2, "Node2Host"}} {
		guest := newTestClient(t, n.address)
		w := decodeWelcome(t, guest.join(1, 68, "guest", "ABCD"))
		if w.code != "ABCD" || len(w.roster) != 2 || strings.TrimRight(string(w.roster[0].name[:]), "\x00") != n.name {
			t.Fatalf("same code crossed node boundary: %#v", w)
		}
	}
}

func TestNodeInfoMalformed(t *testing.T) {
	for _, p := range [][]byte{nil, {1}, {3, 0}, {1, 0, 0}} {
		_, addr := startTestServer(t, nil)
		c := newTestClient(t, addr)
		c.send(msgCliNodeInfo, p)
		c.expectClosed(time.Second)
	}
}
