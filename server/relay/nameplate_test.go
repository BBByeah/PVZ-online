package main

import (
	"bytes"
	"encoding/binary"
	"testing"
	"time"
)

func TestNameplateFlowWaitAndCurrentEpoch(t *testing.T) {
	s, _, code, host, guest := flowPair(t)
	flowBegin(t, s, code, host, guest)
	s.mu.Lock()
	s.rooms[code].roundWaitMask |= 1 << 2
	s.mu.Unlock()
	for _, id := range []byte{0, 1, 2, 3, 0} {
		body := []byte{2, 1, 1, id}
		guest.send(msgCliFlowGame, flowGameBytes(1, 1, 29, body))
		found := false
		for attempt := 0; attempt < 40; attempt++ {
			frame := host.next(time.Second)
			if frame.typ == msgSrvFlowGame && len(frame.payload) >= 10 && binary.LittleEndian.Uint16(frame.payload[8:]) == 29 {
				if !bytes.Equal(frame.payload[10:], body) {
					t.Fatal("nameplate bytes changed during forwarding")
				}
				found = true
				break
			}
		}
		if !found {
			t.Fatal("waiting player cosmetic update was not forwarded")
		}
	}
	before := s.stats.forwarded.Load()
	for _, body := range [][]byte{{2, 1, 1}, {2, 1, 1, 4}, {2, 1, 2, 1}, {1, 2, 1, 1}} {
		guest.send(msgCliFlowGame, flowGameBytes(1, 1, 29, body))
	}
	guest.send(msgCliFlowGame, flowGameBytes(1, 0, 29, []byte{2, 1, 1, 1}))
	time.Sleep(80 * time.Millisecond)
	if s.stats.forwarded.Load() != before {
		t.Fatal("invalid or previous-epoch nameplate frame forwarded")
	}
}
