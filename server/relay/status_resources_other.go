//go:build !linux

package main

// @pvz-online: Linux builds report OS CPU/RSS. Other development hosts keep these fields
// null rather than presenting Go's reserved address space as resident memory.
func readProcessResources() processResources { return processResources{} }
