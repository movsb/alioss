package main

import (
	"os"
	"path/filepath"
)

// parseExpiration parses expression as expiration
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

func listFiles(dir string) ([]string, error) {
	var files []string
	err := filepath.Walk(dir, func(path string, info os.FileInfo, e error) error {
		if e != nil {
			return filepath.SkipDir
		}

		if !info.IsDir() {
			files = append(files, path)
		}
		return nil
	})
	return files, err
}
