package main

import (
	"crypto/ed25519"
	"crypto/rand"
	"encoding/base64"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"os"
	"pvzrelay/internal/catalogwire"
	"time"
)

func run(args []string) error {
	if len(args) == 0 {
		return fmt.Errorf("usage: relay-catalog keygen|sign|verify [options]")
	}
	f := flag.NewFlagSet(args[0], flag.ContinueOnError)
	private := f.String("private", "", "private key file (keep outside game and relay packages)")
	public := f.String("public", "", "public key file")
	input := f.String("input", "", "JSON node list")
	output := f.String("output", "", "signed catalog output")
	previous := f.String("previous", "", "previous signed catalog; required when extending a deployed directory")
	if err := f.Parse(args[1:]); err != nil {
		return err
	}
	readKey := func(path string, n int) ([]byte, error) {
		b, err := os.ReadFile(path)
		if err != nil {
			return nil, err
		}
		v, err := base64.StdEncoding.DecodeString(string(bytesTrim(b)))
		if err != nil || len(v) != n {
			return nil, fmt.Errorf("invalid key file")
		}
		return v, nil
	}
	switch args[0] {
	case "keygen":
		if *private == "" || *public == "" || *private == *public {
			return fmt.Errorf("distinct -private and -public paths required")
		}
		pub, priv, err := ed25519.GenerateKey(rand.Reader)
		if err != nil {
			return err
		}
		// Refuse overwriting a released identity or an existing private key.
		for _, item := range []struct {
			path string
			data []byte
			mode os.FileMode
		}{{*private, priv, 0600}, {*public, pub, 0644}} {
			file, err := os.OpenFile(item.path, os.O_CREATE|os.O_EXCL|os.O_WRONLY, item.mode)
			if err != nil {
				return err
			}
			_, err = file.WriteString(base64.StdEncoding.EncodeToString(item.data) + "\n")
			closeErr := file.Close()
			if err != nil {
				return err
			}
			if closeErr != nil {
				return closeErr
			}
		}
	case "sign":
		key, err := readKey(*private, ed25519.PrivateKeySize)
		if err != nil {
			return err
		}
		file, err := os.Open(*input)
		if err != nil {
			return err
		}
		defer file.Close()
		decoder := json.NewDecoder(io.LimitReader(file, 32769))
		decoder.DisallowUnknownFields()
		var d catalogwire.Document
		if err = decoder.Decode(&d); err != nil {
			return err
		}
		var extra any
		if decoder.Decode(&extra) != io.EOF {
			return fmt.Errorf("trailing JSON")
		}
		if !time.Now().Before(d.ExpiresAt) {
			return fmt.Errorf("expiration must be in the future")
		}
		blob, err := catalogwire.Sign(d, ed25519.PrivateKey(key))
		if err != nil {
			return err
		}
		if *previous != "" {
			old, err := os.ReadFile(*previous)
			if err != nil {
				return err
			}
			prev, err := catalogwire.Parse(old, ed25519.PrivateKey(key).Public().(ed25519.PublicKey), time.Now(), true)
			if err != nil {
				return err
			}
			if err = catalogwire.Transition(prev, d); err != nil {
				return err
			}
		}
		if *output == "" {
			return fmt.Errorf("-output is required")
		}
		return os.WriteFile(*output, blob, 0644)
	case "verify":
		pub, err := readKey(*public, ed25519.PublicKeySize)
		if err != nil {
			return err
		}
		blob, err := os.ReadFile(*input)
		if err != nil {
			return err
		}
		d, err := catalogwire.Parse(blob, pub, time.Now(), false)
		if err != nil {
			return err
		}
		return json.NewEncoder(os.Stdout).Encode(d)
	default:
		return fmt.Errorf("unknown command")
	}
	return nil
}
func bytesTrim(b []byte) []byte {
	for len(b) > 0 && (b[len(b)-1] == '\n' || b[len(b)-1] == '\r' || b[len(b)-1] == ' ') {
		b = b[:len(b)-1]
	}
	return b
}
func main() {
	if err := run(os.Args[1:]); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
