package main

import (
	"encoding/binary"
	"math"
	"time"
)

// Public, bounded capacity metadata. Administration stays on the loopback HTTP
// endpoint; this reply carries no room identities, addresses or credentials.
const (
	msgCliNodeInfo = 0xF01D
	msgSrvNodeInfo = 0xF00E
)

func (s *Server) handleNodeInfo(c *Conn, p []byte) {
	if len(p) != 2 || (binary.LittleEndian.Uint16(p) != 1 && binary.LittleEndian.Uint16(p) != 2) {
		s.dropBadControl(c, msgCliNodeInfo)
		return
	}
	if c.room.Load() != nil {
		return
	}
	v := s.sampleStatus(time.Now())
	var out buf
	version := binary.LittleEndian.Uint16(p)
	out.u16(version)
	out.u16(protocolVersion)
	out.u16(uint16(v.Rooms))
	out.u16(uint16(v.RoomLimit))
	out.u16(uint16(v.Players))
	out.u16(uint16(v.Connections))
	out.u16(uint16(v.ConnectionLimit))
	out.u16(3) // directory v2 and at-least-two-online start rules
	if version == 2 {
		state := uint16(0)
		if v.Operations.AcceptNewRooms {
			state |= 1
		}
		if v.Operations.Busy {
			state |= 2
		}
		if v.Operations.SendBudgetMbps > 0 {
			state |= 8
		}
		out.u16(state)
		out.u16(uint16(v.Operations.Score * 1000))
		out.u16(v.Operations.Weight)
		out.u16(0)
		age := time.Since(v.SampledAt).Milliseconds()
		if age < 0 {
			age = 0
		}
		if age > 60000 {
			age = 60000
		}
		out.b = binary.LittleEndian.AppendUint32(out.b, uint32(age))
		out.b = binary.LittleEndian.AppendUint32(out.b, boundedRate(v.SentBytesPerSecond*8))
		out.b = binary.LittleEndian.AppendUint32(out.b, boundedRate(v.Operations.SendBudgetMbps*1000000))
		out.b = binary.LittleEndian.AppendUint64(out.b, v.Operations.ConfigRevision)
	}
	s.enqueueAndClose(c, encodeFrame(msgSrvNodeInfo, out.b))
}

func boundedRate(value float64) uint32 {
	if value <= 0 || math.IsNaN(value) {
		return 0
	}
	if value >= math.MaxUint32 {
		return math.MaxUint32
	}
	return uint32(value)
}
