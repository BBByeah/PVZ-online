package main

import (
	"bytes"
	"encoding/binary"
	"testing"
	"time"
)

func admissionCredential(t *testing.T, c *testClient) []byte {
	t.Helper()
	f := c.next(time.Second)
	if f.typ != msgSrvCredential || len(f.payload) != 16 || bytes.Equal(f.payload, make([]byte, 16)) {
		t.Fatalf("missing random credential: %#x %x", f.typ, f.payload)
	}
	return f.payload
}
func admissionGate(request, epoch uint32, open bool) []byte {
	p := make([]byte, 9)
	binary.LittleEndian.PutUint32(p, request)
	binary.LittleEndian.PutUint32(p[4:], epoch)
	if open {
		p[8] = 1
	}
	return p
}
func gateAck(t *testing.T, c *testClient, request uint32) byte {
	t.Helper()
	f := c.next(time.Second)
	if f.typ != msgSrvGate || len(f.payload) != 10 || binary.LittleEndian.Uint32(f.payload) != request {
		t.Fatalf("gate ACK mismatch %#x %x", f.typ, f.payload)
	}
	return f.payload[9]
}
func TestAdmissionGateAndVersion(t *testing.T) {
	_, addr := startTestServer(t, nil)
	host := newTestClient(t, addr)
	w := decodeWelcome(t, host.create(1, secureBuild, "host"))
	admissionCredential(t, host)
	old := newTestClient(t, addr)
	old.send(msgCliJoinRoom, joinPayload(1, 66, "old", w.code))
	old.expectReject(rejectBuildMismatch)
	host.send(msgCliGate, admissionGate(1, 2, false))
	if mask := gateAck(t, host, 1); mask != 2 {
		t.Fatalf("mask %x", mask)
	}
	guest := newTestClient(t, addr)
	guest.send(msgCliJoinRoom, joinPayload(1, secureBuild, "guest", w.code))
	guest.expectReject(rejectJoinClosed)
	host.send(msgCliGate, admissionGate(2, 2, true))
	gateAck(t, host, 2)
	guest.join(1, secureBuild, "guest", w.code)
	admissionCredential(t, guest)
	host.next(time.Second)
	guest.send(msgCliGate, admissionGate(100, 2, false))
	guest.expectNothing(20 * time.Millisecond)
	host.send(msgCliGate, admissionGate(3, 2, false))
	if mask := gateAck(t, host, 3); mask != 6 {
		t.Fatalf("joined seat omitted from close ACK: %x", mask)
	}
	host.send(msgCliGate, admissionGate(2, 2, true))
	host.expectNothing(20 * time.Millisecond)
	late := newTestClient(t, addr)
	late.send(msgCliJoinRoom, joinPayload(1, secureBuild, "late", w.code))
	late.expectReject(rejectJoinClosed)
}
func TestAdmissionProtectedRejoin(t *testing.T) {
	_, addr := startTestServer(t, nil)
	host := newTestClient(t, addr)
	w := decodeWelcome(t, host.create(1, secureBuild, "host"))
	admissionCredential(t, host)
	guest := newTestClient(t, addr)
	guest.join(1, secureBuild, "same-name", w.code)
	credential := admissionCredential(t, guest)
	host.next(time.Second)
	host.send(msgCliGate, admissionGate(1, 2, false))
	gateAck(t, host, 1)
	impostor := newTestClient(t, addr)
	impostor.send(msgCliRejoin, rejoinPayload(1, secureBuild, "same-name", w.code, 2))
	impostor.expectReject(rejectRejoinSeatMismatch)
	bad := append(rejoinPayload(1, secureBuild, "same-name", w.code, 2), make([]byte, 16)...)
	impostor.send(msgCliSecureRejoin, bad)
	impostor.expectReject(rejectRejoinSeatMismatch)
	guest.nc.Close()
	if f := host.next(time.Second); f.typ != msgSrvPeerOffline {
		t.Fatal("seat not held")
	}
	back := newTestClient(t, addr)
	back.send(msgCliSecureRejoin, append(rejoinPayload(1, secureBuild, "same-name", w.code, 2), credential...))
	if f := back.next(time.Second); f.typ != msgSrvWelcome {
		t.Fatalf("credential rejoin rejected at closed gate %#x", f.typ)
	}
	if next := admissionCredential(t, back); !bytes.Equal(credential, next) {
		t.Fatal("rejoin credential changed")
	}
	if f := host.next(time.Second); f.typ != msgSrvPeerBack {
		t.Fatal("missing peer back")
	}
}
func TestAdmissionCloseJoinSerialization(t *testing.T) {
	for iteration := 0; iteration < 8; iteration++ {
		_, addr := startTestServer(t, nil)
		host := newTestClient(t, addr)
		w := decodeWelcome(t, host.create(1, secureBuild, "host"))
		admissionCredential(t, host)
		guest := newTestClient(t, addr)
		done := make(chan struct{})
		go func() {
			guest.nc.Write(encodeFrame(msgCliJoinRoom, joinPayload(1, secureBuild, "guest", w.code)))
			close(done)
		}()
		host.send(msgCliGate, admissionGate(1, 2, false))
		<-done
		mask := byte(0)
		for mask == 0 {
			f := host.next(time.Second)
			if f.typ == msgSrvGate {
				mask = f.payload[9]
			} else if f.typ != msgSrvPeerJoin {
				t.Fatalf("unexpected %#x", f.typ)
			}
		}
		f := guest.next(time.Second)
		if f.typ == msgSrvWelcome {
			admissionCredential(t, guest)
			if mask != 6 {
				t.Fatal("accepted arrival missing from close mask")
			}
		} else if f.typ == msgSrvReject {
			if f.payload[2] != rejectJoinClosed || mask != 2 {
				t.Fatal("rejected arrival included in mask")
			}
		} else {
			t.Fatalf("unexpected arrival response %#x", f.typ)
		}
	}
}
func TestAdmissionCancelPending(t *testing.T) {
	_, addr := startTestServer(t, nil)
	host := newTestClient(t, addr)
	w := decodeWelcome(t, host.create(1, secureBuild, "host"))
	admissionCredential(t, host)
	guest := newTestClient(t, addr)
	guest.join(1, secureBuild, "pending", w.code)
	admissionCredential(t, guest)
	host.next(time.Second)
	host.send(msgCliCancelAdmission, []byte{2})
	if f := guest.next(time.Second); f.typ != msgSrvRoomClosed {
		t.Fatal("pending client not informed")
	}
	if f := host.next(time.Second); f.typ != msgSrvPeerLeave {
		t.Fatal("pending seat not released")
	}
	host.send(msgCliGate, admissionGate(1, 2, false))
	if mask := gateAck(t, host, 1); mask != 2 {
		t.Fatal("cancel created a held seat")
	}
	host.send(msgCliGate, admissionGate(2, 2, true))
	gateAck(t, host, 2)
	replacement := newTestClient(t, addr)
	v := decodeWelcome(t, replacement.join(1, secureBuild, "new", w.code))
	if v.yourSeat != 2 {
		t.Fatal("canceled seat not reusable")
	}
	admissionCredential(t, replacement)
}
