package alioss

import (
	"crypto/hmac"
	"crypto/sha1"
	"encoding/base64"
	"fmt"
)

// AccessKey is the key-secret pair that is used to
// authorize the requests.
type AccessKey struct {
	Key    string
	Secret string
}

// sign signs msg with key key.secret
func sign(key *AccessKey, msg string) string {
	h := hmac.New(sha1.New, []byte(key.Secret))
	h.Write([]byte(msg))
	return base64.StdEncoding.EncodeToString(h.Sum(nil))
}

// signHead signs Header fields
func signHead(key *AccessKey, verb string, date string, resource string) string {
	msg := verb + "\n" + "\n" + "\n" + date + "\n" + resource
	mac := sign(key, msg)
	return "OSS " + key.Key + ":" + mac
}

// signURL signs temporary URL
func signURL(key *AccessKey, expiration uint, resource string) string {
	msg := "GET\n\n\n" + fmt.Sprint(expiration) + "\n" + resource
	return sign(key, msg)
}
