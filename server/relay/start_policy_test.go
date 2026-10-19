package main

import (
	"bytes"
	"testing"
	"time"
)

// Relay understands seats and start authority, not the versioned game snapshot.
// Both old clients (44 bytes) and the next rules snapshot (45) retain every byte.
func TestStartSnapshotLengthsForwardWithoutRewriting(t *testing.T) {
	_, addr := startTestServer(t, nil)
	host, guest := newTestClient(t, addr), newTestClient(t, addr)
	welcome := decodeWelcome(t, host.create(protocolVersion, 82, "host"))
	decodeWelcome(t, guest.join(protocolVersion, 82, "guest", welcome.code))
	host.next(time.Second)
	for _, length := range []int{44, 45} {
		payload := make([]byte, length)
		for i := range payload {
			payload[i] = byte(i * 7)
		}
		payload[0], payload[1] = 1, 2
		host.send(3, payload)
		got := guest.next(time.Second)
		for got.typ >= srvFrameBase {
			got = guest.next(time.Second)
		}
		if got.typ != 3 || !bytes.Equal(got.payload, payload) {
			t.Fatalf("snapshot length %d changed: %#v", length, got)
		}
	}
}

func TestServerRoomCountsLivePlayers(t *testing.T) {
	host, guest := &Conn{}, &Conn{}
	room := &Room{host: host, hostSeat: 1}
	room.bySeat[1], room.bySeat[2] = host, guest
	if room.connectedPlayers(false) != 2 {
		t.Fatal("two connected players must count")
	}
	room.roundWaitMask = 1 << 2
	if room.connectedPlayers(false) != 1 || room.connectedPlayers(true) != 2 {
		t.Fatal("waiting players only join the next round")
	}
	room.bySeat[2] = nil
	room.holds[2] = seatHold{until: time.Now().Add(time.Minute)}
	if room.connectedPlayers(true) != 1 {
		t.Fatal("offline held seats must not count")
	}
	room.bySeat[2] = guest
	guest.left.Store(true)
	if room.connectedPlayers(true) != 1 {
		t.Fatal("leaving connections must not count")
	}
	guest.left.Store(false)
	guest.replaced.Store(true)
	if room.connectedPlayers(true) != 1 {
		t.Fatal("replaced connections must not count")
	}
}

func TestServerRoomRejectsSoloAndGuestStarts(t *testing.T) {
	server, addr := startTestServer(t, nil)
	host := newTestClient(t, addr)
	welcome := decodeWelcome(t, host.create(protocolVersion, 16, "host"))
	host.send(3, []byte{1, 2, 0, 0})
	time.Sleep(30 * time.Millisecond)
	server.mu.Lock()
	started := server.rooms[welcome.code].started
	server.mu.Unlock()
	if started {
		t.Fatal("solo start to an empty seat must not start the room")
	}
	guest := newTestClient(t, addr)
	decodeWelcome(t, guest.join(protocolVersion, 16, "guest", welcome.code))
	host.next(time.Second)
	guest.send(3, []byte{2, 1, 0, 0})
	host.expectNothing(100 * time.Millisecond)
	guest.send(14, []byte{2, 1})
	host.expectNothing(100 * time.Millisecond)
	host.send(3, []byte{1, 2, 0, 0})
	if got := guest.next(time.Second); got.typ != 3 {
		t.Fatalf("two-player host start: %#v", got)
	}
	host.send(27, []byte{1, 2, 1, 0, 0, 0, 1 << 2})
	if got := guest.next(time.Second); got.typ != 27 {
		t.Fatal("round wait must reach peer")
	}
	host.send(14, []byte{1, 2})
	guest.expectNothing(100 * time.Millisecond)
	host.send(3, []byte{1, 2, 0, 0})
	if got := guest.next(time.Second); got.typ != 3 {
		t.Fatal("next round must release waiting players")
	}
	host.send(14, []byte{1, 2})
	if got := guest.next(time.Second); got.typ != 14 {
		t.Fatal("two active players can proceed")
	}
}
