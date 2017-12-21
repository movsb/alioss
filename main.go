package main

var key = xAccessKey{
	key:    "",
	secret: "",
}

const (
	ossBucket   = "twofei-test"
	ossLocation = "oss-cn-shenzhen"
)

func main() {
	c := newClient("https://" + ossRootServer)
	c.ListBuckets()
}
