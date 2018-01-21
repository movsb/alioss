package alioss

import (
	"errors"
)

// ErrError
var ErrError = errors.New("unspecified rror")

// ErrBucketNotExist
var ErrBucketNotExist = errors.New("bucket not exist")

// ErrArgs
var ErrArgs = errors.New("invalid arguments")

// ErrNotFound
var ErrNotFound = errors.New("not found")

// ErrExists
var ErrExists = errors.New("already exists")

// OSSError reports an error that is produced by network or oss server
type OSSError struct {
	err error
	msg string
}

func (e OSSError) Error() string {
	s := ""
	if e.err != nil {
		s += e.err.Error()
		s += "\n"
	}
	if e.msg != "" {
		s += e.msg
		s += "\n"
	}
	return s
}
