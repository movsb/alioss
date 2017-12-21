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

func sign(key *xAccessKey, msg string) string {
	h := hmac.New(sha1.New, []byte(key.secret))
	h.Write([]byte(msg))
	// log.Println("sign msg:", msg)
	return base64.StdEncoding.EncodeToString(h.Sum(nil))
}

func signHead(key *xAccessKey, verb string, date string, resource string) string {
	msg := verb + "\n" + "\n" + "\n" + date + "\n" + resource
	mac := sign(key, msg)
	return "OSS " + key.key + ":" + mac
}

func signURL(key *xAccessKey, expiration int, resource string) string {
	msg := "GET\n\n\n" + strconv.Itoa(expiration) + "\n" + resource
	return sign(key, msg)
}
