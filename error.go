package main

import (
	"encoding/xml"
)

/*
<Error>
	<Code>AccessDenied</Code>
	<Message>Anonymous user has no right to access this bucket.</Message>
	<RequestId>5A4FC8CD85566F6D751FD7D8</RequestId>
	<HostId>twofei-test.oss-cn-shenzhen.aliyuncs.com</HostId>
</Error>
*/

type xError struct {
	Code      string
	Message   string
	RequestID string
	HostID    string
}

func parseErrorXML(body []byte) (xerr *xError, err error) {
	xerr = &xError{}
	err = xml.Unmarshal(body, xerr)
	return
}
