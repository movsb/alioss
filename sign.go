package main

import (
	"crypto/hmac"
	"crypto/sha1"
	"encoding/base64"
	"strconv"
)

type xAccessKey struct {
	key    string
	secret string
}

// sign signs msg with key key.secret
func sign(key *xAccessKey, msg string) string {
	h := hmac.New(sha1.New, []byte(key.secret))
	h.Write([]byte(msg))
	return base64.StdEncoding.EncodeToString(h.Sum(nil))
}

// signHead signs Header fields
func signHead(key *xAccessKey, verb string, date string, resource string) string {
	msg := verb + "\n" + "\n" + "\n" + date + "\n" + resource
	mac := sign(key, msg)
	return "OSS " + key.key + ":" + mac
}

// signURL signs temporary URL
func signURL(key *xAccessKey, expiration int, resource string) string {
	msg := "GET\n\n\n" + strconv.Itoa(expiration) + "\n" + resource
	return sign(key, msg)
}
