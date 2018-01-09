package alioss

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
