package main

import (
	"net"
	"testing"
	"time"
)

func legacyQueueSnapshot(wrapped bool) []byte {
	body := []byte{1, 2, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 5}
	if wrapped {
		r := &Room{flow: roundFlow{term: 1, epoch: 1}}
		return flowEnvelope(r, 18, body)
	}
	return encodeFrame(18, body)
}
func queueConn(t *testing.T) (*Server, *Conn, net.Conn) {
	t.Helper()
	s := newServer(t.Logf)
	a, b := net.Pipe()
	c := newConn(s, a)
	t.Cleanup(func() { c.close(); b.Close() })
	return s, c, b
}
func TestLegacySnapshotQueueSaturationPreservesCritical(t *testing.T) {
	for _, wrapped := range []bool{false, true} {
		t.Run(map[bool]string{false: "plain", true: "flow"}[wrapped], func(t *testing.T) {
			s, c, _ := queueConn(t)
			frame := legacyQueueSnapshot(wrapped)
			for i := 0; i < 300; i++ {
				s.enqueue(c, frame)
			}
			select {
			case <-c.done:
				t.Fatal("display-only snapshot burst disconnected game")
			default:
			}
			if !s.enqueue(c, encodeFrame(16, []byte{1, 2, 1})) {
				t.Fatal("no reserved slot for critical gameplay")
			}
			if len(c.send) > 64 {
				t.Fatal("unbounded send queue")
			}
		})
	}
}
func TestLegacySnapshotAtFullCriticalQueueDoesNotDisconnect(t *testing.T) {
	s, c, _ := queueConn(t)
	for i := 0; i < 64; i++ {
		if !s.enqueue(c, encodeFrame(16, []byte{1, 2, 1})) {
			t.Fatal("critical queue filled too early")
		}
	}
	if s.enqueue(c, legacyQueueSnapshot(true)) {
		t.Fatal("snapshot should be dropped")
	}
	select {
	case <-c.done:
		t.Fatal("optional snapshot closed full critical link")
	default:
	}
	if s.enqueue(c, encodeFrame(16, []byte{1, 2, 1})) {
		t.Fatal("critical overflow policy changed")
	}
	select {
	case <-c.done:
	default:
		t.Fatal("critical overflow no longer closes stalled link")
	}
}
func TestLegacySnapshotSlowWriterRecoversControl(t *testing.T) {
	s, c, peer := queueConn(t)
	done := make(chan struct{})
	go func() { defer close(done); c.writeLoop() }()
	t.Cleanup(func() {
		c.close()
		peer.Close()
		select {
		case <-done:
		case <-time.After(time.Second):
			t.Error("owned writer did not exit")
		}
	})
	for i := 0; i < 300; i++ {
		s.enqueue(c, legacyQueueSnapshot(true))
	}
	select {
	case <-c.done:
		t.Fatal("temporarily blocked receiver disconnected")
	default:
	}
	if !s.enqueue(c, encodeFrame(16, []byte{1, 2, 1})) {
		t.Fatal("control rejected during observer pressure")
	}
	peer.SetReadDeadline(time.Now().Add(2 * time.Second))
	for i := 0; i < 66; i++ {
		typ, _, err := readFrame(peer)
		if err != nil {
			t.Fatal(err)
		}
		if typ == 16 {
			return
		}
	}
	t.Fatal("critical frame never reached recovering receiver")
}
