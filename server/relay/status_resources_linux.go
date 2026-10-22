//go:build linux

// @pvz-online: Read OS resource counters because Go's reserved memory is not RSS.
package main

import (
	"os"
	"strconv"
	"strings"
	"syscall"
)

func readProcessResources() processResources {
	var out processResources
	var usage syscall.Rusage
	if syscall.Getrusage(syscall.RUSAGE_SELF, &usage) == nil {
		seconds := float64(usage.Utime.Sec+usage.Stime.Sec) + float64(usage.Utime.Usec+usage.Stime.Usec)/1000000
		out.CPUSeconds = &seconds
	}
	if data, err := os.ReadFile("/proc/self/statm"); err == nil {
		fields := strings.Fields(string(data))
		if len(fields) > 1 {
			if pages, err := strconv.ParseUint(fields[1], 10, 64); err == nil {
				bytes := pages * uint64(os.Getpagesize())
				out.RSSBytes = &bytes
			}
		}
	}
	if data, err := os.ReadFile("/proc/meminfo"); err == nil {
		for _, line := range strings.Split(string(data), "\n") {
			fields := strings.Fields(line)
			if len(fields) < 2 {
				continue
			}
			if fields[0] != "MemTotal:" && fields[0] != "MemAvailable:" {
				continue
			}
			kb, err := strconv.ParseUint(fields[1], 10, 64)
			if err != nil {
				continue
			}
			bytes := kb * 1024
			if fields[0] == "MemTotal:" {
				out.HostMemoryTotalBytes = &bytes
			} else {
				out.HostMemoryAvailableBytes = &bytes
			}
		}
	}
	if data, err := os.ReadFile("/proc/loadavg"); err == nil {
		fields := strings.Fields(string(data))
		if len(fields) > 0 {
			if load, err := strconv.ParseFloat(fields[0], 64); err == nil {
				out.HostLoad1 = &load
			}
		}
	}
	return out
}
