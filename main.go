package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"time"
)

var key = xAccessKey{
	key:    "",
	secret: "",
}

const (
	ossBucket   = "twofei-test"
	ossLocation = "oss-cn-shenzhen"
)

func normalizeSlash(path string) string {
	return strings.Replace(path, "\\", "/", -1)
}

func dirName(path string) string {
	i := strings.LastIndexByte(path, '/')
	if i == -1 {
		return "/"
	}

	return path[:i+1]
}

func findFile(files []File, file string) bool {
	for _, f := range files {
		if f.Key == file {
			return true
		}
	}
	return false
}

func findFolder(folders []Folder, folder string) bool {
	for _, f := range folders {
		if string(f) == folder {
			return true
		}
	}
	return false
}

func help() {
	s := `alioss - a simple Ali OSS manager
 
Syntax:
    <type>   <command>    <[parameters...]>
    bucket   list

    object   list         <directory>
    object   head         <file>
    object   sign         <file>              [expiration]

    object   download     <file>              [file/directory]
    object   download     <directory>         [directory]

    object   upload       <file>              <file>
    object   upload       <directory>         <file>
    object   upload       <directory>         <directory>

	object   delete       <file/directory>
`

	fmt.Fprintln(os.Stderr, s)
}

var oss *Client

func eval(argc int, argv []string) {
	if argc < 1 {
		return
	}

	var err error

	object := argv[0]

	if object == "bucket" {
		if argc >= 2 {
			cmd := argv[1]
			if cmd == "list" {
				bkts := oss.ListBuckets()
				for _, bkt := range bkts {
					fmt.Println(bkt)
				}
			}
		}
	} else if object == "object" {
		if argc >= 2 {
			cmd := argv[1]
			if cmd == "list" {
				if argc >= 3 {
					folder := argv[2]
					files, folders := oss.ListFolder(folder, false)
					if len(files) > 0 {
						fmt.Println("Files:")
						for _, file := range files {
							fmt.Println(file)
						}
					}

					if len(folders) > 0 {
						fmt.Println("Folders:")
						for _, folder := range folders {
							fmt.Println(folder)
						}
					}
				}
			} else if cmd == "head" {
				if argc >= 3 {
					file := argv[2]
					head := oss.HeadObject(file)
					fmt.Println(head)
				}
			} else if cmd == "sign" {
				if argc >= 3 {
					file := argv[2]
					link := "https://"
					link += makePublicHost(ossBucket, ossLocation)

					if argc >= 4 {
						parseExpiration := func(expr string) int {
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

						expr := parseExpiration(argv[3])
						if expr == -1 {
							panic("bad expiration")
						}

						current := int(time.Now().UTC().Unix())
						expires := current + expr

						link, err = makeURL(link, file, map[string]string{
							"OSSAccessKeyId": key.key,
							"Expires":        strconv.Itoa(expires),
							"Signature":      signURL(&key, expires, file),
						})

						if err != nil {
							panic(err)
						}
					} else {
						link, err = makeURL(link, file, nil)
						if err != nil {
							panic(err)
						}
					}

					fmt.Println(link)
				}
			} else if cmd == "download" {
				if argc >= 3 {
					inputPath := normalizeSlash(argv[2])
					inputDir := dirName(inputPath)

					if inputPath[0] != '/' {
						panic("invalid path")
					}

					files, folders := oss.ListFolder(inputDir, false)

					isFileDownload := inputPath[len(inputPath)-1] != '/'

					if isFileDownload && !findFile(files, inputPath) {
						isFileDownload = false
					}

					if !isFileDownload {
						inputPath2 := inputPath
						if inputPath[len(inputPath)-1] != '/' {
							inputPath2 += "/"
						}

						hasFolder := inputPath2 == "/" || findFolder(folders, inputPath2)
						if !hasFolder {
							fmt.Fprintln(os.Stderr, "No such file or folder:", inputPath)
							return
						}

						inputDir = inputPath2
					}

					if isFileDownload {
						localDir := "."
						localName := filepath.Dir(inputPath)

						if argc >= 4 {
							str := normalizeSlash(argv[3])
							if fi, err := os.Stat(str); err == nil && fi.IsDir() || str[len(str)-1] == '/' {
								localDir = str
								if localDir[len(localDir)-1] == '/' {
									localDir = localDir[:len(localDir)-1]
								}
							} else {
								localDir = filepath.Dir(str)
								localName = filepath.Base(str)
							}
						}

						os.MkdirAll(localDir, os.ModeDir)

						localPath := ""
						if localDir[len(localDir)-1] == '/' {
							localPath = localDir + localName
						} else {
							localPath = localDir + "/" + localName
						}

						fp, err := os.Create(localPath)
						if err != nil {
							panic(err)
						}

						err = oss.GetFile(inputPath, fp)
						if err != nil {
							panic(err)
						}
					} else {
						localDir := "."

						if argc >= 4 {
							str := normalizeSlash(argv[3])
							if fi, err := os.Stat(str); err == nil && !fi.IsDir() {
								fmt.Fprintln(os.Stderr, str, "exists, and is not a directory. aborting.")
								return
							}
							localDir = str
							if localDir[len(localDir)-1] == '/' {
								localDir = localDir[:len(localDir)-1]
							}
						}

						files, folders := oss.ListFolder(inputDir, true)

						prefix := inputDir
						if inputDir[len(inputDir)-1] != '/' {
							prefix += "/"
						}

						fmt.Print("Summary: ")
						fmt.Print(len(files), "file")
						if len(files) > 1 {
							fmt.Print("s")
						}

						fmt.Print(len(folders), "folders")
						if len(folders) > 1 {
							fmt.Print("s")
						}

						fmt.Println()

						if len(folders) > 0 {
							fmt.Println()
							for _, folder := range folders {
								path := localDir + "/" + string(folder)[len(prefix):]
								fmt.Printf("  Making directory `%s' ...", path)
								err = os.MkdirAll(path, os.ModeDir)
								if err != nil {
									panic(err)
								}
								fmt.Println("  Done.")
							}
						}

						if len(files) > 0 {
							fmt.Println()
							for _, file := range files {
								path := localDir + "/" + file.Key[len(prefix):]
								fp, err := os.Create(path)
								if err != nil {
									panic(err)
								}
								fmt.Printf("  Downloading `%s' ...", file.Key)
								err = oss.GetFile(file.Key, fp)
								if err != nil {
									panic(err)
								}
								fmt.Println("  Done.")
							}
						}
					}
				}
			} else if cmd == "upload" {

			}
		}
	}
}

func repl() {
	help()
}

func main() {
}
