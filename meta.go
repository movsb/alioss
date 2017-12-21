package main

const (
	ossRootServer   = "oss.aliyuncs.com"
	ossServerSuffix = ".aliyuncs.com"
)

func makeEndpoint(bucketLocation string) string {
	return bucketLocation + ossServerSuffix
}

func makePublicHost(bucketName, bucketLocation string) string {
	return bucketName + "." + bucketLocation + ossServerSuffix
}

type Owner struct {
	ID          string
	DisplayName string
}

type Bucket struct {
	Name         string
	Location     string
	CreationDate string
}

type File struct {
	key          string
	lastModified string
	eTag         string
	typeName     string
	size         string
	storageClass string
	owner        Owner
}

type Folder struct {
	key string
}
