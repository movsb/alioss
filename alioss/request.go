package alioss

import (
	"io"
	"io/ioutil"
	"net/http"
	"net/url"
	"time"
)

func gmdate() string {
	loc, _ := time.LoadLocation("GMT")
	tim := time.Now().In(loc).Format(time.RFC1123)
	return tim
}

type xRequest struct {
	key    *xAccessKey
	host   string
	bucket string
	c      http.Client
}

func newRequest(host string, bucket string, key *xAccessKey) *xRequest {
	r := &xRequest{}
	r.host = host
	r.key = key
	if bucket != "" {
		r.bucket = "/" + bucket
	}
	return r
}

// See https://github.com/aliyun/aliyun-oss-csharp-sdk/blob/cb96bae541499850c185d120f5147b3c64138d8c/sdk/Util/HttpUtils.cs#L22
// See https://github.com/aliyun/aliyun-oss-csharp-sdk/blob/cb96bae541499850c185d120f5147b3c64138d8c/sdk/Util/OssUtils.cs#L179
func escapePath(path string) string {
	s := make([]byte, len(path)*3)
	j := 0
	for i := 0; i < len(path); i++ {
		c := path[i]
		if '0' <= c && c <= '9' || 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z' || c == '/' || c == '-' || c == '_' || c == '.' || c == '~' {
			s[j] = c
			j++
		} else {
			s[j+0] = '%'
			s[j+1] = "0123456789ABCDEF"[c>>4]
			s[j+2] = "0123456789ABCDEF"[c&15]
			j += 3
		}
	}
	return string(s[0:j])
}

func makeURL(host, resource string, queries map[string]string) (string, error) {
	u, err := url.Parse(host)
	if err != nil {
		return "", err
	}

	u.Path = resource
	u.RawPath = escapePath(resource)

	q := u.Query()
	for k, v := range queries {
		q.Add(k, v)
	}

	u.RawQuery = q.Encode()

	return u.String(), nil
}

func (r *xRequest) Do(method string, resource string, queries map[string]string, rc io.ReadCloser, w io.Writer) (*http.Response, []byte, error) {
	u, err := makeURL(r.host, resource, queries)
	if err != nil {
		return nil, nil, err
	}
	req, err := http.NewRequest(method, u, nil)
	if err != nil {
		return nil, nil, err
	}

	req.Body = rc

	date := gmdate()
	req.Header.Add("Date", date)

	auth := signHead(r.key, method, date, r.bucket+resource)
	req.Header.Add("Authorization", auth)

	resp, err := r.c.Do(req)
	if err != nil {
		return nil, nil, err
	}

	defer resp.Body.Close()

	if w == nil || resp.StatusCode != 200 {
		body, err := ioutil.ReadAll(resp.Body)
		if err != nil {
			return nil, nil, err
		}
		return resp, body, nil
	}

	_, err = io.Copy(w, resp.Body)
	return resp, nil, err
}
