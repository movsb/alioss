package main

import (
	"encoding/xml"
	"log"
)

type Client struct {
	req *xRequest
}

func newClient(host string) *Client {
	c := &Client{}
	c.req = newRequest(host)
	return c
}

func (c *Client) ListBuckets() {
	resp, body, err := c.req.Get("/")
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

	//og.Println(string(body))

	bkts := &ListAllMyBucketsResult{}
	err = xml.Unmarshal(body, bkts)
	if err != nil {
		panic(err)
	}

	log.Printf("%s\n", bkts)
}
