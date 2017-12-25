package main

import (
	"encoding/xml"
	"fmt"
	"io"
	"log"
)

// Client represents an alioss client
type Client struct {
	root *xRequest
}

func newClient(host string) *Client {
	c := &Client{}
	c.root = newRequest(host, "")
	return c
}

// ListBuckets lists all buckets that owned by current
// key/secret pair owner
func (c *Client) ListBuckets() []Bucket {
	resp, body, err := c.root.GetString("/", nil)
	if err != nil {
		panic(err)
	}

	if resp.StatusCode != 200 {
		xerr, err := parseErrorXML(body)
		if err != nil {
			panic(err)
		}
		log.Fatalln(xerr)
	}

	type ListAllMyBucketsResult struct {
		Owner   Owner
		Buckets []Bucket `xml:"Buckets>Bucket"`
	}

	bkts := &ListAllMyBucketsResult{}
	err = xml.Unmarshal(body, bkts)
	if err != nil {
		panic(err)
	}

	return bkts.Buckets
}

func (c *Client) listObjectsInternal(prefix, marker string, recursive bool, files *[]File, folders *[]Folder, nextMarker *string, prefixes *[]string) bool {
	req := newRequest("https://"+makePublicHost(ossBucket, ossLocation), ossBucket)
	delimiter := ""
	if !recursive {
		delimiter = "/"
	}

	resp, body, err := req.GetString("/", map[string]string{
		"prefix":    prefix,
		"delimiter": delimiter,
		"max-keys":  "1000",
		"marker":    marker,
	})

	if err != nil {
		panic(err)
	}

	if resp.StatusCode != 200 {
		xerr, err := parseErrorXML(body)
		if err != nil {
			panic(err)
		}
		log.Fatalln(xerr)
	}

	type ListObjectsResult struct {
		IsTruncated    bool
		NextMarker     string
		Contents       []Object
		CommonPrefixes []string
	}

	rets := &ListObjectsResult{}
	err = xml.Unmarshal(body, rets)
	if err != nil {
		panic(err)
	}

	isTruncated := rets.IsTruncated
	if isTruncated {
		*nextMarker = rets.NextMarker
	}

	for _, obj := range rets.Contents {
		if !recursive || obj.isFile() {
			*files = append(*files, obj.File)
		}

		if recursive {
			if obj.isFile() {
				*prefixes = append(*prefixes, obj.dirName())
			} else {
				*prefixes = append(*prefixes, "/"+obj.Key)
			}
		}
	}

	if recursive {
		for _, folder := range rets.CommonPrefixes {
			*folders = append(*folders, Folder(folder))
		}
	}

	return isTruncated
}

func (c *Client) listObjectsLoop(prefix string, recursive bool) ([]File, []Folder) {
	var (
		marker   string
		prefixes []string
		files    []File
		folders  []Folder
	)

	for c.listObjectsInternal(prefix, marker, recursive, &files, &folders, &marker, &prefixes) {
		// emply block
		log.Println("looping")
	}

	if recursive {
		for _, f := range prefixes {
			folders = append(folders, Folder(f))
		}
	}

	return files, folders
}

// ListPrefix lists
func (c *Client) ListPrefix(prefix string) ([]File, []Folder) {
	if prefix == "" || prefix[0] != '/' {
		panic("invalid path.")
	}

	return c.listObjectsLoop(prefix[1:], true)
}

// ListFolder lists files and folders that inside it
func (c *Client) ListFolder(folder string, recursive bool) ([]File, []Folder) {
	if folder == "" || folder[0] != '/' {
		panic("invalid path.")
	}

	prefix := folder[1:]
	if prefix != "" && prefix[len(prefix)-1] != '/' {
		prefix += "/"
	}

	return c.listObjectsLoop(prefix, recursive)
}

// DeleteObject deletes an object
func (c *Client) DeleteObject(obj string) {
	if obj == "" || obj[0] != '/' {
		panic("invalid path")
	}

	req := newRequest("https://"+makePublicHost(ossBucket, ossLocation), ossBucket)
	resp, body, err := req.Delete(obj)

	if err != nil {
		panic(err)
	}

	if resp.StatusCode != 204 && resp.StatusCode != 200 {
		xerr, err := parseErrorXML(body)
		if err != nil {
			panic(err)
		}
		panic(xerr)
	}
}

// HeadObject heads an object
func (c *Client) HeadObject(obj string) string {
	if obj == "" || obj[0] != '/' {
		panic("invalid path")
	}

	req := newRequest("https://"+makePublicHost(ossBucket, ossLocation), ossBucket)
	head, err := req.Head(obj)
	if err != nil {
		panic(err)
	}

	s := ""
	for k, v := range head {
		s += fmt.Sprintf("%-24s: %s\n", k, v[0])
	}

	return s
}

// GetFile gets file contents and writes to w
func (c *Client) GetFile(file string, w io.Writer) error {
	req := newRequest("https://"+makePublicHost(ossBucket, ossLocation), ossBucket)
	err := req.GetFile(file, w)
	return err
}

// PutFile puts a file from rc
func (c *Client) PutFile(file string, rc io.ReadCloser) error {
	req := newRequest("https://"+makePublicHost(ossBucket, ossLocation), ossBucket)
	err := req.PutFile(file, rc)
	return err
}
