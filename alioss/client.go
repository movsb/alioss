package alioss

import (
	"encoding/xml"
	"fmt"
	"io"
)

// Client represents an alioss client
type Client struct {
	key      AccessKey
	bucket   string
	location string
	req      *xRequest
}

// NewClient creates a new client that can be used to
// access all functionalities privided by this library
// to issue requests to the alioss server.
func NewClient(bucket, location, key, secret string) *Client {
	c := &Client{
		key:      AccessKey{Key: key, Secret: secret},
		bucket:   bucket,
		location: location,
	}
	return c
}

// GetEndpoint returns the Endpoint
// with current bucket and location
func (c *Client) GetEndpoint() string {
	return "https://" + c.bucket + "." + c.location + ossServerSuffix
}

func (c *Client) newRequest() *xRequest {
	if c.req == nil {
		c.req = newRequest(c.GetEndpoint(), c.bucket, &c.key)
	}

	return c.req
}

func (c *Client) listObjectsInternal(
	prefix string,
	marker string,
	recursive bool,
	files *[]File,
	folders *[]Folder,
	nextMarker *string,
	prefixes *map[string]bool,
) error {
	if prefix != "" && prefix[len(prefix)-1] != '/' {
		return ErrArgs
	}

	req := c.newRequest()

	// delimiter delimites the objects' full path
	// into common prefixes (namely, the folders)
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
		return err
	}

	if resp.StatusCode == 403 {
		return ErrBucketNotExist
	}

	if resp.StatusCode != 200 {
		return &OSSError{msg: string(body)}
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

	if len(rets.Contents) == 0 && len(rets.CommonPrefixes) == 0 {
		return ErrNotFound
	}

	lenPrefix := len(prefix)

	for _, obj := range rets.Contents {
		obj.Key = obj.Key[lenPrefix:]
		// The folder itself is also returned
		if obj.Key == "" {
			continue
		}

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

	delete(*prefixes, "")

	if !recursive {
		for _, folder := range rets.CommonPrefixes {
			*folders = append(*folders, Folder(folder.Prefix[lenPrefix:]))
		}
	}

	if rets.IsTruncated {
		*nextMarker = rets.NextMarker
		return nil
	}

	return io.EOF
}

func (c *Client) listObjectsLoop(prefix string, recursive bool) ([]File, []Folder, error) {
	var (
		marker   string
		prefixes map[string]bool
		files    []File
		folders  []Folder
	)

	prefixes = make(map[string]bool)

	var err error

	for err == nil {
		err = c.listObjectsInternal(
			prefix, marker, recursive,
			&files, &folders, &marker, &prefixes,
		)
		switch err {
		case nil:
			continue
		case io.EOF:
			break
		default:
			return nil, nil, err
		}
	}

	if recursive {
		for f := range prefixes {
			folders = append(folders, Folder(f))
		}
	}

	return files, folders, nil
}

// ListFolder lists files and folders that inside it
func (c *Client) ListFolder(folder string, recursive bool) ([]File, []Folder, error) {
	if folder == "" {
		return nil, nil, ErrArgs
	}

	prefix := folder

	if prefix[0] == '/' {
		prefix = prefix[1:]
	}

	if prefix != "" && prefix[len(prefix)-1] != '/' {
		prefix += "/"
	}

	return c.listObjectsLoop(prefix, recursive)
}

// DeleteObject deletes an object.
// It can be either a file or a folder (no recursion)
// If the file doesn't exist, DeleteObject will return nil
// To determine the existence of a file, call FileExists
func (c *Client) DeleteObject(obj string) error {
	// cannot delete /
	if obj == "" || obj == "/" {
		return ErrArgs
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

// DeleteFolder recursively deletes objects(files and folders) in it
// On deleting every object, it calls callback with the obj name
func (c *Client) DeleteFolder(folder string, callback func(obj string) bool) error {
	if folder == "" {
		return ErrArgs
	}

	if folder[len(folder)-1] != '/' {
		folder += "/"
	}

	var err error

	files, folders, err := c.ListFolder(folder, true)
	if err != nil {
		return err
	}

	for _, file := range files {
		if callback(file.Key) {
			if err = c.DeleteObject(folder + file.Key); err != nil {
				return err
			}
		}
	}

	for _, f := range folders {
		if callback(string(f)) {
			if err = c.DeleteObject(folder + string(f)); err != nil {
				return err
			}
		}
	}

	if folder != "/" && callback(folder) {
		err = c.DeleteObject(folder)
		if err != nil {
			return err
		}
	}

	return nil
}

// HeadObject heads an object
func (c *Client) HeadObject(obj string) (int, string, error) {
	req := c.newRequest()
	resp, _, err := req.Do("HEAD", obj, nil, nil, nil)
	if err != nil {
		return 0, "", err
	}

	s := ""
	for k, v := range resp.Header {
		s += fmt.Sprintf("%-24s: %s\n", k, v[0])
	}

	return resp.StatusCode, s, nil
}

// FileExists returns nil if `file` is a file and it doest exist
// It calls HeadObject
func (c *Client) FileExists(file string) error {
	// must be a file path, not folder
	if file == "" || file[len(file)-1] == '/' {
		return ErrArgs
	}

	code, _, err := c.HeadObject(file)
	if err != nil {
		return err
	}

	switch code {
	case 200:
		return nil
	case 404:
		return ErrNotFound
	default:
		return ErrError
	}
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
	if path == "" {
		return ErrArgs
	}

	if path == "/" {
		return nil
	}

	if path[0] != '/' {
		path = "/" + path
	}

	if path[len(path)-1] != '/' {
		path += "/"
	}

	if c.FileExists(path[0:len(path)-1]) == nil {
		return ErrExists
	}

	req := c.newRequest()
	resp, body, err := req.Do("PUT", path, nil, nil, nil)

	if err != nil {
		return err
	}

	if resp.StatusCode != 200 {
		return &OSSError{msg: string(body)}
	}

	return nil
}

// MakeShare creates a sharing link with expiration
// set to expiration seconds
func (c *Client) MakeShare(resource string, expiration uint) string {
	link := ""

	if expiration > 0 {
		link, _ = makeURL(c.GetEndpoint(), resource, map[string]string{
			"OSSAccessKeyId": c.key.Key,
			"Expires":        fmt.Sprint(expiration),
			"Signature":      signURL(&c.key, expiration, "/"+c.bucket+resource),
		})
	} else {
		link, _ = makeURL(c.GetEndpoint(), resource, nil)
	}

	return link
}
