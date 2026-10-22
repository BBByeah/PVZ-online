package main

import (
	"net"
	"testing"
	"time"
)

func TestHostedRelayLifecycle(t *testing.T) {
	for i := 0; i < 8; i++ {
		h, err := startHostedRelay("127.0.0.1:0", true, nil)
		if err != nil {
			t.Fatal(err)
		}
		addr := h.listener.Addr().String()
		c := newTestClient(t, addr)
		decodeWelcome(t, c.create(1, 88, "host"))
		if _, err := startHostedRelay(addr, true, nil); err == nil {
			t.Fatal("occupied port accepted")
		}
		h.Close()
		h.Close()
		ln, err := net.Listen("tcp4", addr)
		if err != nil {
			t.Fatalf("port leaked: %v", err)
		}
		ln.Close()
		count := 0
		h.server.connections.Range(func(_, _ any) bool { count++; return true })
		if count != 0 {
			t.Fatal("connection leaked")
		}
	}
}

func TestPrivateSixSeatsAndGracefulHostElection(t *testing.T) {
	h, err := startHostedRelay("127.0.0.1:0", true, nil)
	if err != nil {
		t.Fatal(err)
	}
	defer h.Close()
	s, addr := h.server, h.listener.Addr().String()
	players := make([]*testClient, 6)
	code := ""
	for i := range players {
		players[i] = newTestClient(t, addr)
		if i == 0 {
			code = decodeWelcome(t, players[i].create(1, 88, "host")).code
		} else {
			decodeWelcome(t, players[i].join(1, 88, "guest", code))
		}
		players[i].send(msgCliFlow, []byte{1, 0})
	}
	flowWait(t, s, code, func(r *Room) bool {
		for i := 1; i <= 6; i++ {
			if r.bySeat[i] == nil || !r.bySeat[i].flow {
				return false
			}
		}
		return true
	})
	extra := newTestClient(t, addr)
	extra.send(msgCliJoinRoom, joinPayload(1, 88, "seventh", code))
	extra.expectReject(rejectRoomFull)
	players[0].send(msgCliLeaveRoom, nil)
	flowWait(t, s, code, func(r *Room) bool {
		return r.hostSeat == 2 && r.host == r.bySeat[2] && r.flow.term == 2 && r.bySeat[1] == nil && !r.holdActive(1, time.Now())
	})
	// The host device's service survives its player's explicit departure.
	newcomer := newTestClient(t, addr)
	decodeWelcome(t, newcomer.join(1, 88, "replacement", code))
	flowWait(t, s, code, func(r *Room) bool { return r.bySeat[1] != nil && r.hostSeat == 2 })
}

func TestHostedStopNotifiesGuests(t *testing.T) {
	h, err := startHostedRelay("127.0.0.1:0", true, nil)
	if err != nil {
		t.Fatal(err)
	}
	c := newTestClient(t, h.listener.Addr().String())
	decodeWelcome(t, c.create(1, 88, "host"))
	h.Close()
	// WELCOME is followed by capability/credential frames, which may already
	// be queued before shutdown. The final frame must still be ROOM_CLOSED.
	for i := 0; i < 12; i++ {
		f := c.next(time.Second)
		if f.typ == msgSrvRoomClosed {
			return
		}
	}
	t.Fatal("intentional shutdown did not send ROOM_CLOSED")
}
