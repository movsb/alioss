package main

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"time"
)

var (
	config = make(map[string]string)
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

    object   list         <directory>
    object   mkdir        <directory>
    object   head         <file>
    object   sign         <file>              [expiration]

    object   download     <file>              [file/directory]
    object   download     <directory>         [directory]

    object   upload       <directory>         <files.../directories...>

    object   delete       <file/directory>
`

	fmt.Fprintln(os.Stderr, s)
}

var oss *Client

func eval(argv []string) {
	argc := len(argv)

	if argc < 1 {
		return
	}

	var err error

	object := argv[0]

	if object == "object" {
		if argc >= 2 {
			cmd := argv[1]
			if cmd == "list" {
				if argc >= 3 {
					folder := argv[2]
					files, folders := oss.ListFolder(folder, false)
					if len(files) > 0 {
						fmt.Println("Files:")
						for _, file := range files {
							fmt.Println(" ", &file)
						}
					}

					if len(folders) > 0 {
						fmt.Println("Folders:")
						for _, folder := range folders {
							fmt.Println(" ", folder)
						}
					}
				}
			} else if cmd == "mkdir" {
				if argc >= 3 {
					path := argv[2]
					err := oss.CreateFolder(path)
					if err != nil {
						panic(err)
					} else {
						fmt.Println("Created.")
					}
				}
			} else if cmd == "head" {
				if argc >= 3 {
					file := argv[2]
					status, head, err := oss.HeadObject(file)
					if err != nil {
						panic(err)
					}
					fmt.Println("Status: ", status)
					fmt.Println(head)
				}
			} else if cmd == "sign" {
				if argc >= 3 {
					file := argv[2]
					expr := 0

					if argc >= 4 {
						expr = parseExpiration(argv[3])
						if expr == -1 {
							panic("bad expiration")
						}

						current := int(time.Now().UTC().Unix())
						expr += current
					}

					link := oss.MakeShare(file, expr)

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
						localName := filepath.Base(inputPath)

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

						os.MkdirAll(localDir, os.ModePerm)

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

						defer fp.Close()

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
						fmt.Print(len(files), " file")
						if len(files) > 1 {
							fmt.Print("s")
						}

						fmt.Print(" and ", len(folders), " folder")
						if len(folders) > 1 {
							fmt.Print("s")
						}

						fmt.Println()

						if len(folders) > 0 {
							fmt.Println()
							for _, folder := range folders {
								path := localDir + "/" + string(folder)[len(prefix):]
								fmt.Printf("  Making directory `%s' ...", path)
								err = os.MkdirAll(path, os.ModePerm)
								if err != nil {
									panic(err)
								}
								fmt.Println("  Done.")
							}

							fmt.Println()
						}

						if len(files) > 0 {
							for _, file := range files {
								path := localDir + "/" + file.Key[len(prefix):]
								fp, err := os.Create(path)
								if err != nil {
									panic(err)
								}
								defer fp.Close()
								fmt.Printf("  Downloading `%s' ...", file.Key)
								err = oss.GetFile(file.Key, fp)
								if err != nil {
									panic(err)
								}
								fmt.Println("  Done.")
							}

							fmt.Println()
						}
					}
				}
			} else if cmd == "upload" {
				if argc >= 4 {
					dst := normalizeSlash(argv[2])
					if dst[0] != '/' {
						panic("bad remote path")
					}

					// dst is always a folder
					// so prefix it a /
					if dst[len(dst)-1] != '/' {
						dst += "/"
					}

					for _, src := range argv[3:] {
						src = normalizeSlash(src)

						var statSrc os.FileInfo
						if statSrc, err = os.Stat(src); os.IsNotExist(err) {
							log.Println("No such file or directory")
							continue
						}

						if !statSrc.IsDir() {
							remotePath := dst + filepath.Base(src)

							fp, err := os.Open(src)
							if err != nil {
								panic(err)
							}

							fmt.Printf("Uploading `%s' ...", src)
							err = oss.PutFile(remotePath, fp)
							if err != nil {
								panic(err)
							}
							fmt.Println(" Done.")
						} else if statSrc.IsDir() {
							prefix, files, err := listFiles(src)
							if err != nil {
								panic(err)
							}

							fmt.Print("Summary: ", len(files), " file")
							if len(files) > 1 {
								fmt.Print("s")
							}
							fmt.Println(" will be uploaded.")

							for _, file := range files {
								fp, err := os.Open(prefix + "/" + file)
								if err != nil {
									panic(err)
								}
								defer fp.Close()
								fmt.Printf("  Uploading `%s' ...", file)
								err = oss.PutFile(dst+file, fp)
								if err != nil {
									panic(err)
								}
								fmt.Println(" Done.")
							}
						}
					}
				}
			} else if cmd == "delete" {
				if argc >= 3 {
					spec := normalizeSlash(argv[2])
					files, folders := oss.ListPrefix(spec)
					if findFile(files, spec) {
						if err = oss.DeleteObject(spec); err != nil {
							panic(err)
						}
						fmt.Println("Deleted.")
						return
					}

					specBack := spec
					if specBack[len(specBack)-1] != '/' {
						spec += "/"
					}

					if findFolder(folders, spec) {
						for _, file := range files {
							if strings.HasPrefix(file.Key, spec) {
								fmt.Printf("Deleting `%s' ...", file.Key)
								if err = oss.DeleteObject(file.Key); err != nil {
									panic(err)
								}
								fmt.Println(" Done.")
							}
						}

						// Deleting / is deleting a bucket
						if spec != "/" {
							fmt.Printf("Deleting `%s' ...", spec)
							if err = oss.DeleteObject(spec); err != nil {
								panic(err)
							}
							fmt.Println(" Done.")
						}
						return
						return
					}

					fmt.Fprintf(os.Stderr, "No such file or directory: `%s'.\n", specBack)
					return
				}
			}
		}
	}
}

func repl() {
	help()

	scn := bufio.NewScanner(os.Stdin)
	for {
		fmt.Print("$ ")
		if !scn.Scan() {
			if scn.Err() == nil {
				fmt.Println()
				break
			} else {
				panic(scn.Err())
			}
		}
		line := scn.Text()
		argv := strings.Fields(line)

		if len(argv) <= 0 {
			continue
		}

		if argv[0] == "!" {
			if len(argv) >= 2 {
				name := ""
				args := []string{}
				if runtime.GOOS == "windows" {
					name = "cmd"
					args = append(args, "/c")
					args = append(args, argv[1:]...)
				} else {
					name = "sh"
					args = append(args, "-c")
					args = append(args, argv[1:]...)
				}

				cmd := exec.Command(name, args...)
				out, err := cmd.CombinedOutput()
				if err != nil {
					log.Println(err)
				} else {
					fmt.Print(string(out))
				}
			}
		} else {
			eval(argv)
		}
	}
}

func readConfig() {
	file, err := os.Open("./alioss.conf")
	if err != nil {
		panic("config file alioss.conf cannot be found.")
	}

	defer file.Close()

	scn := bufio.NewScanner(file)

	for scn.Scan() {
		line := scn.Text()
		toks := strings.SplitN(line, ":", 2)
		if len(toks) != 2 {
			continue
		}
		config[toks[0]] = toks[1]
	}
}

func main() {
	readConfig()

	oss = newClient(
		config["bucket"],
		config["location"],
		config["key"],
		config["secret"],
	)

	if len(os.Args) > 1 {
		argv := os.Args[1:]
		eval(argv)
	} else {
		repl()
	}
}
