package main

import (
	"log"
)

var key = xAccessKey{
	key:    "",
	secret: "",
}

const (
	ossBucket   = "twofei-wordpress"
	ossLocation = "oss-cn-hangzhou"
)

func main() {
	oss := newClient(ossRootServer)
	oss.DeleteObject("/a.txt")
	f, d := oss.ListFolder("/code-mirror/", false)
	log.Println(f, d)
}
