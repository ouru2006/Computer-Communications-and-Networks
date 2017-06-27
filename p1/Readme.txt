Assitnment 1 Readme

This project includes a Makefile, serve.c, a index.html in temp/www document and a readme.txt.

The server.c file are a simple UDP-based web Server System. 

1. Go to the file directory and type 'make' on commend line to run the make file.

2. connect to Server
	a) Open a Terminal and type './sws 4000 temp/www/‘ to run the server side. (4000 is the port number, temp/www/ is the path of the index.html file)
	b) Open another Terminal and type “bash Testfile.sh” to send multiple HTTP requests for testing
	c) Also can type’echo -e -n "GET /connex_login.html HTTP/1.0\r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 4000 ' to connect to server and get content of the webpage. connex_login.html is a large file, so that we need to use a loop to send the chunks of the file to the client.
	d) Also can type’echo -e -n "GET /index.html HTTP/1.0\r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 4000 ' to connect to server and get content of index.html. 

3. There are 4 functions under main in server.c
	error()
	Get_time()
	Read_file()
	processRequest()

error() function: print error messages and exit

Get_time() function: get current time and convert it to MMM DD HH:MM:SS form

Read_file() function: Determine the requested file size and reads the file into the buffer

processRequest() function: Processing the received request 
