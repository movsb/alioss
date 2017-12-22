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
	f, d := oss.ListFolder("/code-mirror/", false)
	log.Println(f, d)
}
