package main

import (
	"encoding/binary"
	"time"
)

// Public, bounded capacity metadata. Administration stays on the loopback HTTP
// endpoint; this reply carries no room identities, addresses or credentials.
const (
	msgCliNodeInfo = 0xF01D
	msgSrvNodeInfo = 0xF00E
)

func (s *Server) handleNodeInfo(c *Conn, p []byte) {
	if len(p) != 2 || binary.LittleEndian.Uint16(p) != 1 {
		s.dropBadControl(c, msgCliNodeInfo)
		return
	}
	if c.room.Load() != nil {
		return
	}
	v := s.sampleStatus(time.Now())
	var out buf
	out.u16(1)
	out.u16(protocolVersion)
	out.u16(uint16(v.Rooms))
	out.u16(uint16(v.RoomLimit))
	out.u16(uint16(v.Players))
	out.u16(uint16(v.Connections))
	out.u16(uint16(v.ConnectionLimit))
	out.u16(3) // directory v2 and at-least-two-online start rules
	s.enqueueAndClose(c, encodeFrame(msgSrvNodeInfo, out.b))
}
