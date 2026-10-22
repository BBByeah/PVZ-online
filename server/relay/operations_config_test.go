package main

import (
	"encoding/json"
	"os"
	"path/filepath"
	"testing"
	"time"
)

func writeOps(t *testing.T, path string, c operationsConfig) {
	t.Helper()
	b, err := json.Marshal(c)
	if err != nil {
		t.Fatal(err)
	}
	if err = os.WriteFile(path, b, 0600); err != nil {
		t.Fatal(err)
	}
}
func TestConfigReloadPreservesRoomsAndRejectsInvalid(t *testing.T) {
	s, addr := startTestServer(t, nil)
	path := filepath.Join(t.TempDir(), "operations.json")
	cfg := defaultOperationsConfig()
	cfg.BandwidthMbps = 3
	writeOps(t, path, cfg)
	if err := s.reloadOperations(path); err != nil {
		t.Fatal(err)
	}
	host := newTestClient(t, addr)
	welcome := decodeWelcome(t, host.create(1, 68, "host"))
	host.next(time.Second)
	cfg.Revision++
	cfg.AcceptNewRooms = false
	writeOps(t, path, cfg)
	if err := s.reloadOperations(path); err != nil {
		t.Fatal(err)
	}
	blocked := newTestClient(t, addr)
	blocked.send(msgCliCreateRoom, createPayload(1, 68, "blocked"))
	blocked.expectReject(rejectServerBusy)
	guest := newTestClient(t, addr)
	guest.join(1, 68, "guest", welcome.code)
	guest.next(time.Second)
	host.next(time.Second)
	host.send(42, []byte{1, 2, 77})
	if f := guest.next(time.Second); f.typ != 42 {
		t.Fatal("existing room forwarding changed during reload")
	}
	previous := s.ops.current.Load()
	if err := os.WriteFile(path, []byte(`{"revision":3,"weight":0}`), 0600); err != nil {
		t.Fatal(err)
	}
	if s.reloadOperations(path) == nil || s.ops.current.Load() != previous {
		t.Fatal("invalid config replaced active settings")
	}
	cfg.AcceptNewRooms = true
	writeOps(t, path, cfg)
	if s.reloadOperations(path) == nil {
		t.Fatal("same config revision was mutated")
	}
	cfg.Revision++
	writeOps(t, path, cfg)
	if err := s.reloadOperations(path); err != nil {
		t.Fatal(err)
	}
	next := newTestClient(t, addr)
	next.create(1, 68, "after-reload")
}
func TestConfigWatcherWithoutStatusPage(t *testing.T) {
	s := newServer(t.Logf)
	path := filepath.Join(t.TempDir(), "operations.json")
	cfg := defaultOperationsConfig()
	writeOps(t, path, cfg)
	if err := s.reloadOperations(path); err != nil {
		t.Fatal(err)
	}
	s.startSampling(path)
	t.Cleanup(s.stopSampling)
	cfg.Revision++
	cfg.Weight = 250
	writeOps(t, path, cfg)
	deadline := time.Now().Add(3 * time.Second)
	for time.Now().Before(deadline) && s.operations().Config.Weight != 250 {
		time.Sleep(10 * time.Millisecond)
	}
	if s.operations().Config.Weight != 250 {
		t.Fatal("watcher did not apply config without HTTP")
	}
	if len(s.historySnapshot()) == 0 {
		t.Fatal("history depends on opening status page")
	}
}
func TestConfigBoundsUnknownAndTrailing(t *testing.T) {
	s := newServer(t.Logf)
	path := filepath.Join(t.TempDir(), "operations.json")
	for _, text := range []string{`{"unknown":1}`, `{} {}`, `{"weight":1001}`, `{"bandwidth_mbps":3,"send_budget_mbps":4}`, `{"revision":0}`} {
		if err := os.WriteFile(path, []byte(text), 0600); err != nil {
			t.Fatal(err)
		}
		if s.reloadOperations(path) == nil {
			t.Fatalf("accepted %s", text)
		}
	}
}
