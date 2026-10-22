package catalogwire

import (
	"crypto/ed25519"
	"crypto/rand"
	"testing"
	"time"
)

func fixture() Document {
	return Document{Revision: 1, ExpiresAt: time.Now().Add(time.Hour), Nodes: []Node{{"1", "127.0.0.1", 97, 100, true, true}, {"10", "relay.example", 98, 200, true, true}}}
}
func TestSignatureBoundsAndTransition(t *testing.T) {
	pub, priv, _ := ed25519.GenerateKey(rand.Reader)
	d := fixture()
	blob, err := Sign(d, priv)
	if err != nil {
		t.Fatal(err)
	}
	parsed, err := Parse(blob, pub, time.Now(), false)
	if err != nil || len(parsed.Nodes) != 2 {
		t.Fatal(err)
	}
	blob[30] ^= 1
	if _, err = Parse(blob, pub, time.Now(), false); err == nil {
		t.Fatal("tampered catalog accepted")
	}
	blob[30] ^= 1
	if _, err = Parse(blob[:len(blob)-1], pub, time.Now(), false); err == nil {
		t.Fatal("truncated signature accepted")
	}
	if _, err = Parse(blob, pub, time.Now().Add(2*time.Hour), false); err == nil {
		t.Fatal("expired remote catalog accepted")
	}
	if _, err = Parse(blob, pub, time.Now().Add(2*time.Hour), true); err != nil {
		t.Fatal("verified old local routing cache unavailable")
	}
	next := fixture()
	next.Revision = 2
	next.Nodes[0].Create = false
	if Transition(d, next) != nil {
		t.Fatal("retirement rejected")
	}
	next.Nodes[0].Host = "other.example"
	if Transition(d, next) == nil {
		t.Fatal("room-code redirection accepted")
	}
	next = fixture()
	next.Revision = 2
	next.Nodes = next.Nodes[1:]
	if Transition(d, next) == nil {
		t.Fatal("old code identity removed")
	}
	next = fixture()
	next.Nodes[1].Weight++
	if Transition(d, next) == nil {
		t.Fatal("same revision changed")
	}
}
func TestInvalidCatalogNodes(t *testing.T) {
	for _, change := range []func(*Document){func(d *Document) { d.Nodes[1].ID = "1" }, func(d *Document) { d.Nodes[1].ID = "01" }, func(d *Document) { d.Nodes[1].Host = "x/y" }, func(d *Document) { d.Nodes[1].Port = 0 }, func(d *Document) { d.Nodes[1].Weight = 0 }, func(d *Document) { d.Nodes = make([]Node, MaxNodes+1) }} {
		d := fixture()
		change(&d)
		if _, err := d.Body(); err == nil {
			t.Fatalf("invalid node accepted: %+v", d)
		}
	}
}
