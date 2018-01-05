package main

import (
	"os"
	"path/filepath"
	"runtime"
	"strings"
)

// parseExpiration parses expression as expiration
// Expiration is expressed as:
//		?d?h?m?s
// where:
//		d: day, h: hour, m: minute, s: second
func parseExpiration(expr string) int {
	var day, hour, minute, second int

	for i := 0; i < len(expr); i++ {
		num := 0
		unit := 's'

		for ; i < len(expr); i++ {
			if '0' <= expr[i] && expr[i] <= '9' {
				num *= 10
				num += int(expr[i] - '0')
			} else {
				break
			}
		}

		if i < len(expr) {
			unit = rune(expr[i])
			i++
		}

		var p *int

		switch unit {
		case 'd':
			p = &day
		case 'h':
			p = &hour
		case 'm':
			p = &minute
		case 's':
			p = &second
		default:
			return -1
		}

		*p = num
	}

	expiration := day*(24*60*60) + hour*(60*60) + minute*(60) + second*(1)

	return expiration
}

// Lists  all files (no folders) in a folder
// returns the prefix dir path, files and error
// returned files does not include dir(cleaned) as its prefix
func listFiles(dir string) (string, []string, error) {
	var files []string
	var isWindows = runtime.GOOS == "windows"
	var cleanedDir = filepath.Clean(dir)
	var cleanedDirLen = 0
	if cleanedDir == "." || cleanedDir == "./" || cleanedDir == `.\` {
		cleanedDirLen = 0
	} else {
		cleanedDirLen = len(cleanedDir) + 1
	}
	err := filepath.Walk(dir, func(path string, info os.FileInfo, e error) error {
		if e != nil {
			return filepath.SkipDir
		}

		if !info.IsDir() {
			if isWindows {
				path = strings.Replace(path, `\`, "/", -1)
			}
			files = append(files, path[cleanedDirLen:])
		}
		return nil
	})
	return cleanedDir, files, err
}
