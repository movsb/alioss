package main

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
}
