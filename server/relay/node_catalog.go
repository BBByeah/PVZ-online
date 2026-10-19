package main

import (
	"encoding/binary"
	"time"
)

const msgCliNodeCatalog = 0xF01E
const msgSrvNodeCatalog = 0xF00F
const catalogChunkBytes = 240

func (s *Server) handleNodeCatalog(c *Conn, p []byte) {
	if len(p) != 2 || binary.LittleEndian.Uint16(p) != 1 {
		s.dropBadControl(c, msgCliNodeCatalog)
		return
	}
	if c.room.Load() != nil {
		return
	}
	snapshot := s.operations()
	blob := snapshot.CatalogBlob
	if len(blob) == 0 || !time.Now().Before(snapshot.Catalog.ExpiresAt) {
		// One empty, well-formed response lets the client try another source quickly.
		s.enqueueAndClose(c, encodeFrame(msgSrvNodeCatalog, []byte{1, 0, 0, 0, 0, 0, 0, 0}))
		return
	}
	count := (len(blob) + catalogChunkBytes - 1) / catalogChunkBytes
	for index := 0; index < count; index++ {
		end := (index + 1) * catalogChunkBytes
		if end > len(blob) {
			end = len(blob)
		}
		chunk := blob[index*catalogChunkBytes : end]
		var out buf
		out.u16(1)
		out.u16(uint16(index))
		out.u16(uint16(count))
		out.u16(uint16(len(chunk)))
		out.raw(chunk)
		frame := encodeFrame(msgSrvNodeCatalog, out.b)
		if index+1 == count {
			s.enqueueAndClose(c, frame)
		} else {
			s.enqueue(c, frame)
		}
	}
}
