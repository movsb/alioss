package main

import (
	"fmt"
	"io/ioutil"
	"net/http"
	"net/url"
	"time"
)

func gmdate() string {
	n := time.Now().UTC()
	s := fmt.Sprintf("%s, %02d %s %04d %02d:%02d:%02d GMT",
		n.Format("Sat"),
		n.Day(), n.Format("Jan"), n.Year(),
		n.Hour(), n.Minute(), n.Second(),
	)
	return s
}

type xRequest struct {
	host   string
	bucket string
	c      http.Client
}

func newRequest(host string, bucket string) *xRequest {
	r := &xRequest{}
	r.host = host
	if bucket != "" {
		r.bucket = "/" + bucket
	}
	return r
}

func makeURL(host, resource string, queries map[string]string) (string, error) {
	u, err := url.Parse(host)
	if err != nil {
		return "", err
	}

	u.Path = resource

	q := u.Query()
	for k, v := range queries {
		q.Add(k, v)
	}

	u.RawQuery = q.Encode()

	return u.String(), nil
}

func (r *xRequest) Get(resource string, queries map[string]string) (*http.Response, []byte, error) {
	u, err := makeURL(r.host, resource, queries)
	if err != nil {
		return nil, nil, err
	}
	req, err := http.NewRequest("GET", u, nil)
	if err != nil {
		return nil, nil, err
	}

	date := gmdate()
	req.Header.Add("Date", date)

	auth := signHead(&key, "GET", date, r.bucket+resource)
	req.Header.Add("Authorization", auth)

	resp, err := r.c.Do(req)
	if err != nil {
		return nil, nil, err
	}

	defer resp.Body.Close()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, nil, err
	}

	return resp, body, nil
}
