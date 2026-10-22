package main

import (
	"path/filepath"
	"testing"
	"time"
)

func TestBusyStillAllowsNewRoom(t *testing.T) {
	s, addr := startTestServer(t, nil)
	path := filepath.Join(t.TempDir(), "operations.json")
	cfg := defaultOperationsConfig()
	cfg.BandwidthMbps = 3
	writeOps(t, path, cfg)
	if err := s.reloadOperations(path); err != nil {
		t.Fatal(err)
	}
	now := time.Now()
	s.sampleStatus(now)
	s.stats.outBytes.Add(3000000)
	sample := s.sampleStatus(now.Add(2 * time.Second))
	if !sample.Operations.Busy || !sample.Operations.AcceptNewRooms || sample.Operations.State != "busy" {
		t.Fatalf("soft overload blocked creation: %+v", sample.Operations)
	}
	host := newTestClient(t, addr)
	host.create(1, 68, "busy-still-create")
}
func TestTrendBoundAndRecentWindow(t *testing.T) {
	s := newServer(t.Logf)
	now := time.Now()
	s.sampleStatus(now)
	s.stats.queueDrops.Add(3)
	s.stats.writeErrors.Add(2)
	v := s.sampleStatus(now.Add(5 * time.Second))
	if v.Operations.QueueDropsLastMinute != 3 || v.Operations.WriteErrorsLastMinute != 2 {
		t.Fatal("recent counters lost")
	}
	for i := 2; i < 160; i++ {
		s.sampleStatus(now.Add(time.Duration(i) * 5 * time.Second))
	}
	history := s.historySnapshot()
	if len(history) != historyCapacity {
		t.Fatalf("unbounded/missing history: %d", len(history))
	}
	v = s.sampleStatus(now.Add(801 * time.Second))
	if v.Operations.QueueDropsLastMinute != 0 || v.Operations.WriteErrorsLastMinute != 0 {
		t.Fatal("old cumulative errors remain current")
	}
	history[0].Rooms = 999
	if s.historySnapshot()[0].Rooms == 999 {
		t.Fatal("history reader changed live storage")
	}
}
