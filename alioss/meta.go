package alioss

import (
	"fmt"
	"strings"
)

const (
	ossServerSuffix = ".aliyuncs.com"
)

// Owner represents an owner of a file, a folder, or a bucket
type Owner struct {
	ID          string
	DisplayName string
}

// File represents a file object
type File struct {
	Key          string
	LastModified string
	ETag         string
	Type         string
	Size         string
	StorageClass string
	Owner        Owner
}

// dirName returns the dir name component of a file
// e.g.:/path/to/file.txt -> /path/to
func (f *File) dirName() string {
	i := strings.LastIndexByte(f.Key, '/')
	if i == -1 {
		return ""
	}

	return f.Key[:i+1]
}

// String implements Stringer
func (f *File) String() string {
	return fmt.Sprintf(`Name: %-20s Size: %8s`, f.Key, f.Size)
}

// Folder represents a folder object
type Folder string

// Object maybe a file or a folder
type Object struct {
	File
}

// isFile returns true if o is a file
func (o *Object) isFile() bool {
	return !strings.HasSuffix(o.Key, "/")
}
