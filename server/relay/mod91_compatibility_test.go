package main

import "testing"

func TestMOD91RoomRejectsMOD90(t *testing.T) {
	_, addr := startTestServer(t, nil)
	host := newTestClient(t, addr)
	w := decodeWelcome(t, host.create(1, 91, "mod91-host"))
	admissionCredential(t, host)
	old := newTestClient(t, addr)
	old.send(msgCliJoinRoom, joinPayload(1, 90, "mod90-guest", w.code))
	old.expectReject(rejectBuildMismatch)
	current := newTestClient(t, addr)
	current.join(1, 91, "mod91-guest", w.code)
}
