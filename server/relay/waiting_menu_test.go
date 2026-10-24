package main

import (
	"testing"
	"time"
)

// Public-server mode: interrupted combatants retain their promised holds, but
// a sole online waiting seat can cancel its dead board without claiming a win.
func TestRecoveryOfficialSoleWaitingTCPReturnsLobby(t *testing.T) {
	s, addr, code, c := sixRoom(t)
	c[0].send(msgCliFlowProgress, flowProgressBytes(1, 1, 1, 0, 0))
	flowWait(t, s, code, func(r *Room) bool { return r.flow.phase == flowBattle })
	credential := admissionCredential(t, c[1])
	back := newTestClient(t, addr)
	back.send(msgCliSecureRejoin, append(rejoinPayload(1, 90, "p2", code, 2), credential...))
	decodeWelcome(t, back.next(time.Second).payload)
	flowWait(t, s, code, func(r *Room) bool { return r.roundWaitMask&4 != 0 })
	for i, p := range c {
		if i != 1 {
			p.nc.Close()
		}
	}
	var term uint32
	flowWait(t, s, code, func(r *Room) bool {
		term = r.flow.term
		return r.hostSeat == 2 && r.onlineMask() == 4 && r.flowMask() == 0
	})
	back.send(msgCliFlowProgress, flowProgressBytes(term, 1, 5, 1, 0))
	flowWait(t, s, code, func(r *Room) bool {
		return r.flow.phase == flowLobby && r.roundWaitMask == 0 && r.flow.done == 0 && r.flow.applied == 0
	})
	back.send(msgCliFlowStart, flowStartBytes(term, 2, 2))
	time.Sleep(30 * time.Millisecond)
	s.mu.Lock()
	defer s.mu.Unlock()
	if r := s.rooms[code]; r == nil || r.flow.epoch != 1 || r.flow.phase != flowLobby {
		t.Fatal("sole recovered official player started without teammate")
	}
}
