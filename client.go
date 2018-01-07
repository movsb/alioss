package main

import (
	"encoding/xml"
	"fmt"
	"io"
	"log"
	"strconv"
	"strings"
)

// Client represents an alioss client
type Client struct {
	key      xAccessKey
	bucket   string
	location string
}

func newClient(bucket, location, key, secret string) *Client {
	c := &Client{
		key:      xAccessKey{key: key, secret: secret},
		bucket:   bucket,
		location: location,
	}
	return c
}

// returns the endpoint
func (c *Client) getHost() string {
	return makePublicHost(c.bucket, c.location)
}

func (c *Client) newRequest() *xRequest {
	return newRequest(makePublicHost(c.bucket, c.location), c.bucket, &c.key)
}

func (c *Client) listObjectsInternal(prefix, marker string, recursive bool, files *[]File, folders *[]Folder, nextMarker *string, prefixes *map[string]bool) bool {
	req := c.newRequest()
	delimiter := ""
	if !recursive {
		delimiter = "/"
	}

	resp, body, err := req.Do("GET", "/", map[string]string{
		"prefix":    prefix,
		"delimiter": delimiter,
		"max-keys":  "1000",
		"marker":    marker,
	}, nil, nil)

	if err != nil {
		panic(err)
	}

	if resp.StatusCode != 200 {
		log.Fatalln(string(body))
	}

	type ListObjectsResult struct {
		IsTruncated    bool
		NextMarker     string
		Contents       []Object
		CommonPrefixes []struct {
			Prefix string
		}
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
		obj.Key = "/" + obj.Key

		if !recursive || obj.isFile() {
			*files = append(*files, obj.File)
		}

		if recursive {
			if obj.isFile() {
				(*prefixes)[obj.dirName()] = true
			} else {
				(*prefixes)[obj.Key] = true
			}
		}
	}

	if !recursive {
		for _, folder := range rets.CommonPrefixes {
			*folders = append(*folders, Folder("/"+folder.Prefix))
		}
	}

	return isTruncated
}

func (c *Client) listObjectsLoop(prefix string, recursive bool) ([]File, []Folder) {
	var (
		marker   string
		prefixes map[string]bool
		files    []File
		folders  []Folder
	)

	prefixes = make(map[string]bool)

	for c.listObjectsInternal(prefix, marker, recursive, &files, &folders, &marker, &prefixes) {
		// emply block
	}

	if recursive {
		for f := range prefixes {
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
func (c *Client) DeleteObject(obj string) error {
	// cannot delete /
	if obj == "/" {
		return nil
	}

	req := c.newRequest()
	resp, body, err := req.Do("DELETE", obj, nil, nil, nil)

	if err != nil {
		return &OSSError{err: err}
	}

	if resp.StatusCode != 204 && resp.StatusCode != 200 {
		return &OSSError{msg: string(body)}
	}

	return nil
}

// HeadObject heads an object
func (c *Client) HeadObject(obj string) (int, string, error) {
	req := c.newRequest()
	resp, _, err := req.Do("HEAD", obj, nil, nil, nil)
	if err != nil {
		return 0, "", &OSSError{err: err}
	}

	s := ""
	for k, v := range resp.Header {
		s += fmt.Sprintf("%-24s: %s\n", k, v[0])
	}

	return resp.StatusCode, s, nil
}

// GetFile gets file contents and writes to w
func (c *Client) GetFile(file string, w io.Writer) error {
	req := c.newRequest()
	resp, body, err := req.Do("GET", file, nil, nil, w)

	if err != nil {
		return &OSSError{err: err}
	}

	if resp.StatusCode != 200 {
		return &OSSError{msg: string(body)}
	}

	return nil
}

// PutFile puts a file from rc
func (c *Client) PutFile(file string, rc io.ReadCloser) error {
	req := c.newRequest()
	resp, body, err := req.Do("PUT", file, nil, rc, nil)

	if err != nil {
		return &OSSError{err: err}
	}

	if resp.StatusCode != 200 {
		return &OSSError{msg: string(body)}
	}

	return nil
}

// CreateFolder creates a folder named path
func (c *Client) CreateFolder(path string) error {
	if !strings.HasPrefix(path, "/") {
		return &OSSError{msg: "invalid path: should begin with /"}
	}

	// That folders end with slash is mandatory
	if !strings.HasSuffix(path, "/") {
		path += "/"
	}

	// cannot create root
	if path == "/" {
		return &OSSError{msg: "cannot create root"}
	}

	req := c.newRequest()
	resp, body, err := req.Do("PUT", path, nil, nil, nil)

	if err != nil {
		return &OSSError{err: err}
	}

	if resp.StatusCode != 200 {
		return &OSSError{msg: string(body)}
	}

	return nil
}

// MakeShare creates a sharing link with expiration set to expiration
func (c *Client) MakeShare(resource string, expiration int) string {
	link := ""

	if expiration > 0 {
		link, _ = makeURL(c.getHost(), resource, map[string]string{
			"OSSAccessKeyId": c.key.key,
			"Expires":        strconv.Itoa(expiration),
			"Signature":      signURL(&c.key, expiration, "/"+c.bucket+resource),
		})
	} else {
		link, _ = makeURL(c.getHost(), resource, nil)
	}

	return link
}
