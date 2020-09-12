Author: Yeon Kim
NetID:  ykim22

/bin        contains the compiled binary files
/include    contains header files necessary for the project
/src        contains the main source code for both UDP client and server
/test       contains the three given test files

In the top-most level of the project, running `make` will compile the code and place executables in /bin.
`make clean` will remove the compiled executables.

Example:
`./bin/server 41026`
`./bin/client student06.cse.nd.edu 41026 "some message here"`
`./bin/client student06.cse.nd.edu 41026 ./test/File1.txt`

By default, the client will treat unknown paths as strings. If `./test/File1.txt` does not exist, then the string `"./test/File1.txt"` will be sent to the designated server instead.
