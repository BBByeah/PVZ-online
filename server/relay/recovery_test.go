package main

import (
	"testing"
	"time"
)

func TestFlowRecoveryFixedDeadlineAndNoWaitingExtension(t *testing.T) {
	now := time.Unix(1700000000, 0)
	r := &Room{flow: roundFlow{enabled: true, started: true, phase: flowBattle}, roundWaitMask: 1 << 2}
	r.bySeat[2] = &Conn{}
	r.holds[1].until = now.Add(60 * time.Second)
	r.updateFlowRecovery(now, 60*time.Second)
	deadline := r.flow.recoveryUntil
	if deadline != r.holds[1].until {
		t.Fatal("did not preserve the original hold deadline")
	}
	// The original combatant returning still waits; consuming its hold must
	// neither shorten the promised grace nor grant a win or combat eligibility.
	r.holds[1] = seatHold{}
	r.bySeat[1] = &Conn{}
	r.roundWaitMask |= 1 << 1
	r.updateFlowRecovery(now.Add(20*time.Second), 60*time.Second)
	if r.flow.recoveryUntil != deadline || r.flowMask() != 0 || r.flow.phase != flowBattle {
		t.Fatal("waiting rejoin changed the deadline or combat state")
	}
	// A waiting client dropping again has a newer hold, which cannot keep
	// extending the exhausted-combat room indefinitely.
	r.bySeat[2] = nil
	r.holds[2].until = now.Add(100 * time.Second)
	r.updateFlowRecovery(now.Add(40*time.Second), 60*time.Second)
	if r.flow.recoveryUntil != deadline {
		t.Fatal("waiting reconnect extended the fixed deadline")
	}
}

func TestFlowRecoveryOnlyStrandedBattles(t *testing.T) {
	for _, tc := range []struct {
		name    string
		enabled bool
		started bool
		phase   uint8
		waiting uint8
		want    bool
	}{
		{"battle waiting only", true, true, flowBattle, 4, true},
		{"verify waiting only", true, true, flowVerify, 4, true},
		{"surviving combatant", true, true, flowBattle, 0, false},
		{"legacy", false, true, flowBattle, 4, false},
		{"not started", true, false, flowBattle, 4, false},
		{"setup", true, true, flowSetup, 4, false},
		{"complete", true, true, flowComplete, 4, false},
		{"intermission", true, true, flowIntermission, 4, false},
	} {
		t.Run(tc.name, func(t *testing.T) {
			r := &Room{flow: roundFlow{enabled: tc.enabled, started: tc.started, phase: tc.phase}, roundWaitMask: tc.waiting}
			r.bySeat[2] = &Conn{}
			r.holds[1].until = time.Unix(1700000060, 0)
			r.updateFlowRecovery(time.Unix(1700000000, 0), 60*time.Second)
			if !r.flow.recoveryUntil.IsZero() != tc.want {
				t.Fatal("unexpected recovery timer")
			}
		})
	}
}

func TestFlowRecoveryUsesLatestOriginalHoldAndResetsForActive(t *testing.T) {
	now := time.Unix(1700000000, 0)
	r := &Room{flow: roundFlow{enabled: true, started: true, phase: flowBattle}}
	r.holds[1].until = now.Add(40 * time.Second)
	r.holds[2].until = now.Add(60 * time.Second)
	r.updateFlowRecovery(now, 60*time.Second)
	if r.flow.recoveryUntil != r.holds[2].until {
		t.Fatal("not all original holds were respected")
	}
	r.bySeat[3] = &Conn{}
	r.updateFlowRecovery(now.Add(time.Second), 60*time.Second)
	if !r.flow.recoveryUntil.IsZero() {
		t.Fatal("active battle must not retain an orphan deadline")
	}
}

func TestFlowRecoveryTCPExpiresWithExplicitRestartReason(t *testing.T) {
	s, addr, code, h, g := flowPair(t)
	credential := admissionCredential(t, g)
	flowBegin(t, s, code, h, g)
	g.nc.Close()
	flowWait(t, s, code, func(r *Room) bool { return r.bySeat[2] == nil && r.flowMask() == 2 })
	back := newTestClient(t, addr)
	back.send(msgCliSecureRejoin, append(rejoinPayload(1, 83, "guest", code, 2), credential...))
	w := decodeWelcome(t, back.next(time.Second).payload)
	if w.yourSeat != 2 {
		t.Fatal("wrong reconnect seat")
	}
	flowWait(t, s, code, func(r *Room) bool { return r.bySeat[2] != nil && r.roundWaitMask&4 != 0 })
	h.nc.Close()
	var deadline time.Time
	flowWait(t, s, code, func(r *Room) bool {
		deadline = r.flow.recoveryUntil
		return r.hostSeat == 2 && r.flowMask() == 0 && !deadline.IsZero()
	})
	s.sweepOnce(deadline.Add(-time.Nanosecond))
	s.mu.Lock()
	r := s.rooms[code]
	stillWaiting := r != nil && r.flow.epoch == 1 && r.flow.phase == flowBattle && !r.flow.lost && r.flowMask() == 0
	s.mu.Unlock()
	if !stillWaiting {
		t.Fatal("closed early, auto-won or auto-restarted")
	}
	s.sweepOnce(deadline)
	s.mu.Lock()
	_, exists := s.rooms[code]
	s.mu.Unlock()
	if exists {
		t.Fatal("stranded room was not released after grace")
	}
	for i := 0; i < 64; i++ {
		frame := back.next(time.Second)
		if frame.typ == msgSrvRoomClosed {
			if len(frame.payload) != 1 || frame.payload[0] != roomClosedNoBattlePlayers {
				t.Fatal("missing explicit restart reason")
			}
			return
		}
	}
	t.Fatal("restart prompt frame did not reach the waiting player")
}

func TestFlowRecoveryFastReplacementStillWaitsOneGrace(t *testing.T) {
	s, addr, code, h, g := flowPair(t)
	credential := admissionCredential(t, h)
	flowBegin(t, s, code, h, g)
	// Mark peer as an already-rejoined waiting player, then replace the last
	// active connection before the relay sees a physical disconnect.
	s.mu.Lock()
	r := s.rooms[code]
	r.roundWaitMask = 1 << 2
	s.mu.Unlock()
	back := newTestClient(t, addr)
	back.send(msgCliSecureRejoin, append(rejoinPayload(1, 83, "host", code, 1), credential...))
	decodeWelcome(t, back.next(time.Second).payload)
	flowWait(t, s, code, func(r *Room) bool { return r.flowMask() == 0 && r.bySeat[1] != nil && r.holds[1].until.IsZero() })
	now := time.Now()
	s.sweepOnce(now)
	s.mu.Lock()
	r = s.rooms[code]
	valid := r != nil && r.flow.recoveryUntil.Equal(now.Add(s.holdDuration)) && r.flow.phase == flowBattle && r.flow.epoch == 1
	s.mu.Unlock()
	if !valid {
		t.Fatal("live-link replacement did not get one complete grace window")
	}
	s.sweepOnce(now.Add(s.holdDuration - time.Nanosecond))
	s.mu.Lock()
	_, present := s.rooms[code]
	s.mu.Unlock()
	if !present {
		t.Fatal("replacement room expired early")
	}
	s.sweepOnce(now.Add(s.holdDuration))
	s.mu.Lock()
	_, present = s.rooms[code]
	s.mu.Unlock()
	if present {
		t.Fatal("replacement room did not expire at its fixed deadline")
	}
}
