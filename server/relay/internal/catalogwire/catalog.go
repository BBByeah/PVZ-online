// Package catalogwire defines the signed, bounded node directory shared by the
// maintenance tool and relay. The signing key is never needed on a game node.
package catalogwire

import (
	"bytes"
	"crypto/ed25519"
	"encoding/binary"
	"errors"
	"fmt"
	"strconv"
	"strings"
	"time"
)

const MaxBytes = 8192
const MaxNodes = 64
const Magic = "PVZNODE1"

type Node struct {
	ID        string `json:"id"`
	Host      string `json:"host"`
	Port      uint16 `json:"port"`
	Weight    uint16 `json:"weight"`
	Create    bool   `json:"create"`
	Directory bool   `json:"directory"`
}
type Document struct {
	Revision  uint64    `json:"revision"`
	ExpiresAt time.Time `json:"expires_at"`
	Nodes     []Node    `json:"nodes"`
}

func ValidHost(host string) bool {
	if len(host) == 0 || len(host) > 253 || host[0] == '.' || host[len(host)-1] == '.' {
		return false
	}
	for _, c := range []byte(host) {
		if !(c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c >= '0' && c <= '9' || c == '.' || c == '-') {
			return false
		}
	}
	return true
}
func (d Document) Body() ([]byte, error) {
	if d.Revision == 0 || d.ExpiresAt.Unix() <= 0 || len(d.Nodes) == 0 || len(d.Nodes) > MaxNodes {
		return nil, errors.New("invalid catalog header")
	}
	b := append([]byte{}, []byte(Magic)...)
	b = binary.LittleEndian.AppendUint64(b, d.Revision)
	b = binary.LittleEndian.AppendUint64(b, uint64(d.ExpiresAt.Unix()))
	b = binary.LittleEndian.AppendUint16(b, uint16(len(d.Nodes)))
	ids := map[string]bool{}
	endpoints := map[string]bool{}
	for _, n := range d.Nodes {
		id, err := strconv.ParseUint(n.ID, 10, 16)
		endpoint := strings.ToLower(n.Host) + ":" + strconv.Itoa(int(n.Port))
		if err != nil || id == 0 || strconv.FormatUint(id, 10) != n.ID || ids[n.ID] || endpoints[endpoint] || !ValidHost(n.Host) || n.Port == 0 || n.Weight == 0 || n.Weight > 1000 {
			return nil, fmt.Errorf("invalid or repeated node %q", n.ID)
		}
		ids[n.ID] = true
		endpoints[endpoint] = true
		flags := uint16(0)
		if n.Create {
			flags |= 1
		}
		if n.Directory {
			flags |= 2
		}
		for _, v := range []uint16{uint16(id), flags, n.Weight, n.Port, uint16(len(n.Host))} {
			b = binary.LittleEndian.AppendUint16(b, v)
		}
		b = append(b, []byte(n.Host)...)
	}
	if len(b)+ed25519.SignatureSize > MaxBytes {
		return nil, errors.New("catalog exceeds size limit")
	}
	return b, nil
}
func Sign(d Document, key ed25519.PrivateKey) ([]byte, error) {
	if len(key) != ed25519.PrivateKeySize {
		return nil, errors.New("invalid signing key")
	}
	b, err := d.Body()
	if err != nil {
		return nil, err
	}
	return append(b, ed25519.Sign(key, b)...), nil
}
func Parse(blob []byte, key ed25519.PublicKey, now time.Time, allowExpired bool) (Document, error) {
	var d Document
	if len(blob) < 26+ed25519.SignatureSize || len(blob) > MaxBytes || len(key) != ed25519.PublicKeySize {
		return d, errors.New("invalid catalog size or public key")
	}
	body := blob[:len(blob)-ed25519.SignatureSize]
	if !ed25519.Verify(key, body, blob[len(body):]) {
		return d, errors.New("catalog signature mismatch")
	}
	if string(body[:8]) != Magic {
		return d, errors.New("invalid catalog magic")
	}
	d.Revision = binary.LittleEndian.Uint64(body[8:])
	expiry := binary.LittleEndian.Uint64(body[16:])
	if expiry > uint64(1<<63-1) {
		return d, errors.New("invalid expiration")
	}
	d.ExpiresAt = time.Unix(int64(expiry), 0).UTC()
	if !allowExpired && !now.Before(d.ExpiresAt) {
		return d, errors.New("catalog expired")
	}
	count := int(binary.LittleEndian.Uint16(body[24:]))
	off := 26
	if count == 0 || count > MaxNodes {
		return d, errors.New("invalid node count")
	}
	for i := 0; i < count; i++ {
		if off+10 > len(body) {
			return d, errors.New("truncated node")
		}
		v := func(at int) uint16 { return binary.LittleEndian.Uint16(body[off+at:]) }
		id, flags, weight, port, n := v(0), v(2), v(4), v(6), int(v(8))
		off += 10
		if flags & ^uint16(3) != 0 || n > 253 || off+n > len(body) {
			return d, errors.New("invalid node fields")
		}
		d.Nodes = append(d.Nodes, Node{strconv.Itoa(int(id)), string(body[off : off+n]), port, weight, flags&1 != 0, flags&2 != 0})
		off += n
	}
	if off != len(body) {
		return d, errors.New("trailing catalog data")
	}
	canonical, err := d.Body()
	if err != nil {
		return d, err
	}
	if !bytes.Equal(canonical, body) {
		return d, errors.New("noncanonical catalog")
	}
	return d, nil
}

// Changing an existing ID's endpoint would redirect an old room code. Retiring
// a node changes its flags; its identity and endpoint remain in the directory.
func Transition(previous, next Document) error {
	if next.Revision < previous.Revision {
		return errors.New("catalog revision rollback")
	}
	found := map[string]Node{}
	for _, n := range next.Nodes {
		found[n.ID] = n
	}
	for _, n := range previous.Nodes {
		v, ok := found[n.ID]
		if !ok || !strings.EqualFold(n.Host, v.Host) || n.Port != v.Port {
			return fmt.Errorf("node %s identity removed or redirected", n.ID)
		}
	}
	if next.Revision == previous.Revision {
		a, _ := previous.Body()
		b, _ := next.Body()
		if !bytes.Equal(a, b) {
			return errors.New("same revision changed")
		}
	}
	return nil
}
