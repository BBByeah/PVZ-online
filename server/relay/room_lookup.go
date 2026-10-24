package main

import (
	"encoding/binary"
	"net"
	"time"
)

type lookupRate struct {
	window time.Time
	count  int
}

// Called under Server.mu. An incomplete/limited lookup must close without a
// PAGE; replying found=0 would let the client mistake missing coverage for a
// globally unique match on another server. Memory is bounded even under spray.
func (s *Server) allowRoomLookup(address string, now time.Time) bool {
	key, _, err := net.SplitHostPort(address)
	if err != nil {
		key = address
	}
	if s.lookupRates == nil {
		s.lookupRates = make(map[string]lookupRate)
	}
	if now.Sub(s.lookupWindow) >= time.Second {
		s.lookupWindow = now
		s.lookupCount = 0
	}
	if s.lookupCount >= 128 {
		return false
	}
	// Count attempts, including denied IPs, so a full table cannot be scanned
	// without a global work bound by rotating connections or source addresses.
	s.lookupCount++
	entry, exists := s.lookupRates[key]
	if !exists && len(s.lookupRates) >= 2048 {
		for ip, v := range s.lookupRates {
			if now.Sub(v.window) >= time.Minute {
				delete(s.lookupRates, ip)
			}
		}
		if len(s.lookupRates) >= 2048 {
			return false
		}
	}
	if now.Sub(entry.window) >= time.Second {
		entry = lookupRate{window: now}
	}
	if entry.count >= 8 {
		return false
	}
	entry.count++
	s.lookupRates[key] = entry
	return true
}

func (s *Server) handleRoomLookup(c *Conn, p []byte) {
	if len(p) != 8 || binary.LittleEndian.Uint16(p) != 3 || !validRoomCode(string(p[4:])) {
		s.dropBadControl(c, msgCliList)
		return
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	if c.room.Load() != nil {
		return
	}
	if !s.allowRoomLookup(c.remoteAddr(), time.Now()) {
		s.stats.rateLimited.Add(1)
		c.close()
		return
	}
	s.stats.directoryQueries.Add(1)
	out := make([]byte, 11)
	binary.LittleEndian.PutUint16(out, 3)
	copy(out[3:7], p[4:])
	if r := s.rooms[string(p[4:])]; r != nil {
		now := time.Now()
		players := uint8(0)
		build := uint16(0)
		if r.host != nil {
			build = r.host.build
		} else if r.hostSeat >= 1 && r.hostSeat <= maxPlayers {
			build = r.holds[r.hostSeat].build
		}
		for seat := 1; seat <= maxPlayers; seat++ {
			if peer := r.bySeat[seat]; peer != nil {
				players++
				if build == 0 {
					build = peer.build
				}
			} else if r.holdActive(uint8(seat), now) {
				players++
				if build == 0 {
					build = r.holds[seat].build
				}
			}
		}
		if players > 0 && build > 0 {
			out[2] = 1
			binary.LittleEndian.PutUint16(out[7:], build)
			out[9] = players
			if !r.public {
				out[10] |= 1
			}
			if r.started {
				out[10] |= 2
			}
			if r.host == nil || (r.gateManaged && !r.joinOpen) {
				out[10] |= 4
			}
			if r.lowestFreeSeat(now) == 0 {
				out[10] |= 8
			}
		}
	}
	s.enqueueAndClose(c, encodeFrame(msgSrvDirectory, out))
}
