package main

import (
	"encoding/binary"
	"fmt"
	"testing"
	"time"
)

func sixRoom(t *testing.T) (*Server, string, string, [6]*testClient) {
	t.Helper()
	s, addr := startTestServer(t, nil)
	var c [6]*testClient
	c[0] = newTestClient(t, addr)
	code := decodeWelcome(t, c[0].create(1, 90, "p1")).code
	for i := 1; i < 6; i++ {
		c[i] = newTestClient(t, addr)
		decodeWelcome(t, c[i].join(1, 90, fmt.Sprintf("p%d", i+1), code))
	}
	for _, p := range c {
		p.send(msgCliFlow, []byte{1, 0})
	}
	flowWait(t, s, code, func(r *Room) bool {
		for i := 1; i <= 6; i++ {
			if !r.bySeat[i].flow {
				return false
			}
		}
		return r.flow.enabled
	})
	c[0].send(msgCliFlowStart, flowStartBytes(1, 1, 1))
	flowWait(t, s, code, func(r *Room) bool { return r.flow.epoch == 1 })
	return s, addr, code, c
}

func TestRecoverySixTCPMenuThenStart(t *testing.T) {
	s, _, code, c := sixRoom(t)
	c[0].send(msgCliFlowProgress, flowProgressBytes(1, 1, 1, 0, 0))
	flowWait(t, s, code, func(r *Room) bool { return r.flow.phase == flowBattle })
	// MOD90 existing exit frame must recover the room as well as notify peers.
	c[0].send(msgCliFlowGame, flowGameBytes(1, 1, 13, []byte{1, 2, 0}))
	flowWait(t, s, code, func(r *Room) bool { return r.flow.phase == flowLobby && !r.started && r.roundWaitMask == 0 })
	c[0].send(msgCliFlowStart, flowStartBytes(1, 2, 1))
	flowWait(t, s, code, func(r *Room) bool { return r.flow.epoch == 2 && r.flow.phase == flowSetup })
	for i := 1; i < 6; i++ {
		found := false
		for n := 0; n < 64; n++ {
			f := c[i].next(time.Second)
			if f.typ == msgSrvFlowGame && len(f.payload) >= 12 && binary.LittleEndian.Uint16(f.payload[8:]) == 3 && binary.LittleEndian.Uint32(f.payload[4:]) == 2 {
				found = true
				break
			}
		}
		if !found {
			t.Fatalf("seat%d missed next START", i+1)
		}
	}
}

func TestRecoverySixTCPMenuProgressAndNoFalseWin(t *testing.T) {
	s, _, code, c := sixRoom(t)
	c[0].send(msgCliFlowProgress, flowProgressBytes(1, 1, 5, 1, 0))
	flowWait(t, s, code, func(r *Room) bool { return r.flow.phase == flowLobby && r.flow.done == 0 && r.flow.applied == 0 })
	c[0].send(msgCliFlowProgress, flowProgressBytes(1, 1, 1, 1, 0))
	time.Sleep(20 * time.Millisecond)
	s.mu.Lock()
	r := s.rooms[code]
	ok := r != nil && r.flow.phase == flowLobby && r.flow.done == 0
	s.mu.Unlock()
	if !ok {
		t.Fatal("stale board report mutated menu")
	}
}

func TestRecoveryIntermissionHostRejoinIsNotWaiting(t *testing.T) {
	s, addr, code, c := sixRoom(t)
	credential := admissionCredential(t, c[0])
	s.mu.Lock()
	r := s.rooms[code]
	r.flow.phase = flowIntermission
	s.mu.Unlock()
	back := newTestClient(t, addr)
	back.send(msgCliSecureRejoin, append(rejoinPayload(1, 90, "p1", code, 1), credential...))
	decodeWelcome(t, back.next(time.Second).payload)
	flowWait(t, s, code, func(r *Room) bool { return r.bySeat[1] != nil })
	s.mu.Lock()
	waiting := s.rooms[code].roundWaitMask&2 != 0
	s.mu.Unlock()
	if waiting {
		t.Fatal("intermission host incorrectly waits for next stage")
	}
}

func TestRecoveryOfficialSinglePlayerStillCannotStart(t *testing.T) {
	s, addr := startTestServer(t, nil)
	h := newTestClient(t, addr)
	code := decodeWelcome(t, h.create(1, 90, "solo")).code
	h.send(msgCliFlow, []byte{1, 0})
	flowWait(t, s, code, func(r *Room) bool { return r.flow.enabled && r.flow.term == 1 })
	h.send(msgCliFlowStart, flowStartBytes(1, 1, 1))
	time.Sleep(30 * time.Millisecond)
	s.mu.Lock()
	defer s.mu.Unlock()
	r := s.rooms[code]
	if r.flow.started || r.flow.epoch != 0 {
		t.Fatal("official one-player room started")
	}
}
