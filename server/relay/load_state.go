package main

import (
	"math"
	"time"
)

const historyCapacity = 120

type trendSample struct {
	At                time.Time `json:"at"`
	SentMbps          float64   `json:"sent_mbps"`
	Connections       int       `json:"connections"`
	Rooms             int       `json:"rooms"`
	Queue             int       `json:"queue"`
	CPUPercentOneCore *float64  `json:"cpu_percent_one_core"`
	QueueDrops        uint64    `json:"-"`
	WriteErrors       uint64    `json:"-"`
	Timeouts          uint64    `json:"-"`
}
type operationsStatus struct {
	ConfigRevision        uint64    `json:"config_revision"`
	ConfigLoadedAt        time.Time `json:"config_loaded_at"`
	ConfigError           string    `json:"config_error,omitempty"`
	BandwidthMbps         float64   `json:"bandwidth_mbps"`
	SendBudgetMbps        float64   `json:"send_budget_mbps"`
	Weight                uint16    `json:"weight"`
	AcceptNewRooms        bool      `json:"accept_new_rooms"`
	Busy                  bool      `json:"busy"`
	State                 string    `json:"state"`
	Score                 float64   `json:"score"`
	Reasons               []string  `json:"reasons"`
	QueueDropsLastMinute  uint64    `json:"queue_drops_last_minute"`
	WriteErrorsLastMinute uint64    `json:"write_errors_last_minute"`
	TimeoutsLastMinute    uint64    `json:"timeouts_last_minute"`
	RecentWindowSeconds   float64   `json:"recent_window_seconds"`
	CatalogRevision       uint64    `json:"catalog_revision"`
	CatalogNodes          int       `json:"catalog_nodes"`
	CatalogExpiresAt      time.Time `json:"catalog_expires_at,omitempty"`
	CatalogExpired        bool      `json:"catalog_expired"`
}

func deltaCounter(a, b uint64) uint64 {
	if a < b {
		return 0
	}
	return a - b
}

// Called while status.mu is held. History and recent deltas have a fixed upper
// bound and are sampled independently of HTTP readers in the production entry.
func (s *Server) summarizeOperations(v relayStatus) operationsStatus {
	snapshot := s.operations()
	cfg := snapshot.Config
	budget := cfg.SendBudgetMbps
	if budget == 0 && cfg.BandwidthMbps > 0 {
		budget = cfg.BandwidthMbps * 0.75
	}
	o := operationsStatus{ConfigRevision: cfg.Revision, ConfigLoadedAt: snapshot.LoadedAt, ConfigError: s.configError(), BandwidthMbps: cfg.BandwidthMbps, SendBudgetMbps: budget, Weight: cfg.Weight, AcceptNewRooms: cfg.AcceptNewRooms, State: "normal", Reasons: []string{}, CatalogRevision: snapshot.Catalog.Revision, CatalogNodes: len(snapshot.Catalog.Nodes)}
	if len(snapshot.CatalogBlob) > 0 {
		o.CatalogExpiresAt = snapshot.Catalog.ExpiresAt
		o.CatalogExpired = !v.SampledAt.Before(snapshot.Catalog.ExpiresAt)
	}
	base := trendSample{At: v.SampledAt, QueueDrops: v.QueueDropsTotal, WriteErrors: v.WriteErrorsTotal, Timeouts: v.TimedOutTotal}
	if len(s.status.history) > 0 {
		base = s.status.history[len(s.status.history)-1]
		for _, h := range s.status.history {
			if !h.At.Before(v.SampledAt.Add(-time.Minute)) {
				base = h
				break
			}
		}
		o.RecentWindowSeconds = math.Max(0, v.SampledAt.Sub(base.At).Seconds())
	}
	o.QueueDropsLastMinute = deltaCounter(v.QueueDropsTotal, base.QueueDrops)
	o.WriteErrorsLastMinute = deltaCounter(v.WriteErrorsTotal, base.WriteErrors)
	o.TimeoutsLastMinute = deltaCounter(v.TimedOutTotal, base.Timeouts)
	score := func(value float64, reason string) {
		if value > o.Score {
			o.Score = value
		}
		if value >= 0.8 {
			o.Reasons = append(o.Reasons, reason)
		}
	}
	score(float64(v.Rooms)/float64(roomLimit), "rooms")
	score(float64(v.Connections)/float64(connectionLimit), "connections")
	if budget > 0 {
		score(v.SentBytesPerSecond*8/1e6/budget, "send_budget")
	}
	if v.CPUPercentOneCore != nil && v.CPUs > 0 {
		cores := v.CPUs
		if v.GoMaxProcs > 0 && v.GoMaxProcs < cores {
			cores = v.GoMaxProcs
		}
		score(*v.CPUPercentOneCore/float64(cores)/cfg.BusyCPUPercent, "cpu")
	}
	score(float64(v.MaxConnectionQueue)/64, "send_queue")
	if available := v.Process.HostMemoryAvailableBytes; available != nil {
		score(math.Min(100*1048576/math.Max(1, float64(*available)), 2), "host_memory")
	}
	if o.QueueDropsLastMinute > 0 {
		score(1+math.Min(float64(o.QueueDropsLastMinute)/100, 1), "recent_queue_drops")
	}
	if o.WriteErrorsLastMinute > 0 {
		score(1+math.Min(float64(o.WriteErrorsLastMinute)/100, 1), "recent_write_errors")
	}
	if o.TimeoutsLastMinute > 0 {
		score(1+math.Min(float64(o.TimeoutsLastMinute)/100, 1), "recent_timeouts")
	}
	o.Score = math.Min(o.Score, 65.535)
	o.Busy = o.Score >= 1
	if o.Busy {
		o.State = "busy"
	}
	if v.Rooms >= roomLimit || v.Connections >= connectionLimit {
		o.State = "full"
		o.AcceptNewRooms = false
	}
	if !cfg.AcceptNewRooms {
		o.State = "maintenance"
		o.AcceptNewRooms = false
	}
	h := trendSample{At: v.SampledAt, SentMbps: v.SentBytesPerSecond * 8 / 1e6, Connections: v.Connections, Rooms: v.Rooms, Queue: v.MaxConnectionQueue, CPUPercentOneCore: v.CPUPercentOneCore, QueueDrops: v.QueueDropsTotal, WriteErrors: v.WriteErrorsTotal, Timeouts: v.TimedOutTotal}
	if len(s.status.history) == 0 || v.SampledAt.Sub(s.status.history[len(s.status.history)-1].At) >= 5*time.Second {
		if len(s.status.history) == historyCapacity {
			copy(s.status.history, s.status.history[1:])
			s.status.history = s.status.history[:historyCapacity-1]
		}
		s.status.history = append(s.status.history, h)
	}
	return o
}
func (s *Server) historySnapshot() []trendSample {
	s.status.mu.Lock()
	defer s.status.mu.Unlock()
	return append([]trendSample{}, s.status.history...)
}
