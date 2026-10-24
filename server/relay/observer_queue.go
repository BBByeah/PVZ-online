package main

import "encoding/binary"

// Only an exactly framed legacy type18 fragment is optional. Do not classify
// controls, v2 skeletons or arbitrary malformed frames as disposable gameplay.
func legacyObserverSnapshot(frame []byte) bool {
	if len(frame) < 4 || len(frame) != 4+int(binary.LittleEndian.Uint16(frame[2:])) {
		return false
	}
	typ := binary.LittleEndian.Uint16(frame)
	body := frame[4:]
	if typ == msgSrvFlowGame {
		if len(body) < 16 || binary.LittleEndian.Uint16(body[8:]) != 18 {
			return false
		}
		body = body[10:]
	} else if typ != 18 {
		return false
	}
	return len(body) >= 6 && len(body) <= maxPayload && body[0] >= 1 && body[0] <= maxPlayers && body[1] >= 1 && body[1] <= maxPlayers && body[0] != body[1] && body[5] >= 1 && body[5] <= 8 && body[4] < body[5]
}
