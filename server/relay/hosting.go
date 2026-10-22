package main

import (
	"errors"
	"net"
	"sync"
	"time"
)

// HostedRelay owns every listener, connection and timer of an embedded server.
// It uses the same protocol/room core as the public CLI, without a directory,
// admin endpoint or config watcher. Start failure never leaves a partial host.
type HostedRelay struct {
	server   *Server
	listener net.Listener
	done     chan struct{}
	accepted chan struct{}
	stop     sync.Once
	workers  sync.WaitGroup
}

func startHostedRelay(address string, private bool, logf func(string, ...any)) (*HostedRelay, error) {
	ln, err := net.Listen("tcp4", address)
	if err != nil {
		return nil, err
	}
	if logf == nil {
		logf = func(string, ...any) {}
	}
	s := newServer(logf)
	s.privateHost = private
	s.gamePort = ln.Addr().(*net.TCPAddr).Port
	h := &HostedRelay{server: s, listener: ln, done: make(chan struct{}), accepted: make(chan struct{})}
	go h.accept()
	h.workers.Add(1)
	go func() {
		defer h.workers.Done()
		ticker := time.NewTicker(time.Second)
		defer ticker.Stop()
		for {
			select {
			case now := <-ticker.C:
				s.sweepOnce(now)
				s.sampleStatus(now)
			case <-h.done:
				return
			}
		}
	}()
	return h, nil
}

func (h *HostedRelay) accept() {
	defer close(h.accepted)
	slots := make(chan struct{}, connectionLimit)
	for {
		nc, err := h.listener.Accept()
		if err != nil {
			if errors.Is(err, net.ErrClosed) {
				return
			}
			select {
			case <-h.done:
				return
			default:
			}
			// Back off on transient descriptor exhaustion rather than spinning.
			select {
			case <-h.done:
				return
			case <-time.After(100 * time.Millisecond):
			}
			continue
		}
		select {
		case slots <- struct{}{}:
			c := newConn(h.server, nc)
			h.workers.Add(1)
			go func() { defer h.workers.Done(); defer func() { <-slots }(); c.run() }()
		default:
			h.server.stats.capacityRejected.Add(1)
			nc.Close()
		}
	}
}

func (h *HostedRelay) Close() {
	if h == nil {
		return
	}
	h.stop.Do(func() {
		close(h.done)
		h.listener.Close()
		<-h.accepted // no new workers/connections after this point
		// Publish an explicit room-ended frame before closing sockets, so guests
		// do not spend their reconnect grace retrying an intentionally ended host.
		h.server.mu.Lock()
		for _, room := range h.server.rooms {
			for seat := 1; seat <= maxPlayers; seat++ {
				if c := room.bySeat[seat]; c != nil {
					c.room.Store(nil)
					h.server.enqueueAndClose(c, encodeFrame(msgSrvRoomClosed, encodeRoomClosed(roomClosedHostLeft)))
				}
			}
		}
		h.server.rooms = make(map[string]*Room)
		h.server.mu.Unlock()
		deadline := time.Now().Add(250 * time.Millisecond)
		for time.Now().Before(deadline) {
			remaining := false
			h.server.connections.Range(func(key, _ any) bool {
				if key.(*Conn).room.Load() == nil {
					remaining = true
				}
				return true
			})
			if !remaining {
				break
			}
			time.Sleep(time.Millisecond)
		}
		h.server.connections.Range(func(key, _ any) bool { key.(*Conn).close(); return true })
		h.workers.Wait()
	})
}
