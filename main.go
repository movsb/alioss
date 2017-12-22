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
	s := oss.HeadObject("/vim.js")
	log.Printf("\n%s", s)
}
