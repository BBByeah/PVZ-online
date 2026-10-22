//go:build android

package main

/*
#include <stdlib.h>
*/
import "C"
import (
	"fmt"
	"sync"
)

var mobileHost struct {
	sync.Mutex
	host *HostedRelay
	port int
}

//export PVZHostStart
func PVZHostStart(port C.int) *C.char {
	mobileHost.Lock()
	defer mobileHost.Unlock()
	p := int(port)
	if p < 1 || p > 65535 {
		return C.CString("Port must be between 1 and 65535")
	}
	if mobileHost.host != nil {
		if p == mobileHost.port {
			return nil
		}
		return C.CString("End the existing host before changing its port")
	}
	h, err := startHostedRelay(fmt.Sprintf(":%d", p), true, nil)
	if err != nil {
		return C.CString(err.Error())
	}
	mobileHost.host, mobileHost.port = h, p
	return nil
}

//export PVZHostStop
func PVZHostStop() {
	mobileHost.Lock()
	defer mobileHost.Unlock()
	if mobileHost.host != nil {
		mobileHost.host.Close()
	}
	mobileHost.host, mobileHost.port = nil, 0
}

//export PVZHostPort
func PVZHostPort() C.int {
	mobileHost.Lock()
	defer mobileHost.Unlock()
	return C.int(mobileHost.port)
}
