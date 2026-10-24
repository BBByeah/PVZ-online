package main

import (
	"bytes"
	"crypto/ed25519"
	"crypto/rand"
	"encoding/base64"
	"encoding/binary"
	"os"
	"path/filepath"
	"pvzrelay/internal/catalogwire"
	"strconv"
	"testing"
	"time"
)

func TestCatalogPublicChunksAndAtomicReload(t *testing.T) {
	s, addr := startTestServer(t, nil)
	dir := t.TempDir()
	cfgPath := filepath.Join(dir, "operations.json")
	pub, priv, _ := ed25519.GenerateKey(rand.Reader)
	d := catalogwire.Document{Revision: 1, ExpiresAt: time.Now().Add(time.Hour)}
	for i := 1; i <= 32; i++ {
		d.Nodes = append(d.Nodes, catalogwire.Node{ID: strconv.Itoa(i), Host: "node" + strconv.Itoa(i) + ".example", Port: 97, Weight: 100, Create: true, Directory: true})
	}
	blob, err := catalogwire.Sign(d, priv)
	if err != nil {
		t.Fatal(err)
	}
	catalogPath := filepath.Join(dir, "catalog.bin")
	if err = os.WriteFile(catalogPath, blob, 0644); err != nil {
		t.Fatal(err)
	}
	cfg := defaultOperationsConfig()
	cfg.CatalogPath = "catalog.bin"
	cfg.CatalogPublicKey = base64.StdEncoding.EncodeToString(pub)
	writeOps(t, cfgPath, cfg)
	if err = s.reloadOperations(cfgPath); err != nil {
		t.Fatal(err)
	}
	q := newTestClient(t, addr)
	q.send(msgCliNodeCatalog, []byte{1, 0})
	var result []byte
	count := 0
	for {
		frame := q.next(time.Second)
		if frame.typ != msgSrvNodeCatalog || len(frame.payload) > 256 {
			t.Fatal("wrong catalog frame")
		}
		p := frame.payload
		at := func(n int) uint16 { return binary.LittleEndian.Uint16(p[n:]) }
		if at(0) != 1 || int(at(2)) != count || int(at(6)) != len(p)-8 {
			t.Fatal("inconsistent chunks")
		}
		result = append(result, p[8:]...)
		count++
		if count == int(at(4)) {
			break
		}
	}
	q.expectClosed(time.Second)
	if count < 2 || !bytes.Equal(result, blob) {
		t.Fatal("catalog changed or chunking missing")
	}
	previous := s.ops.current.Load()
	blob[30] ^= 1
	os.WriteFile(catalogPath, blob, 0644)
	if s.reloadOperations(cfgPath) == nil || s.ops.current.Load() != previous {
		t.Fatal("bad signature replaced catalog")
	}
}
func TestNodeInfoV2AndLegacyShape(t *testing.T) {
	_, addr := startTestServer(t, nil)
	for _, version := range []byte{1, 2} {
		q := newTestClient(t, addr)
		q.send(msgCliNodeInfo, []byte{version, 0})
		f := q.next(time.Second)
		want := 16
		if version == 2 {
			want = 44
		}
		if f.typ != msgSrvNodeInfo || len(f.payload) != want {
			t.Fatal("metadata compatibility")
		}
		q.expectClosed(time.Second)
	}
}
