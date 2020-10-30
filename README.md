# HTTP-Client

This is an implementation of basic HTTP client that handles HTTP requests based on user’s command line input.
The request will be translated into an HTTP request and sent to the appropriate server. In response, we will receive the appropriate response for the specific request (HTML page/ header/ error message).

## how to use it
* compiling the program: ```gcc client.c -o client```<br>
client is the executable program.
* the usage has to be in the following format:
```./client [-h] [-d <time-Interval>] <URL>```
  * The format of the URL has to be:  ```http://hostname[:port]/filepath```
  * The format of time request has to be: ```<day>:<hr>:<min>```

the arguments can come in any order but the time interval should go with the '-d' argument.

## what the program does
  * Parse the given in the command line.
  * Connect to the server.
  * Construct an HTTP request based on the options specified in the command line.
  * Send the HTTP request to server.
  * Receive an HTTP response.
  * Display the response on the screen.

## submitted files
  * <b>client.c</b><br>
	This file contains a main that receiving array of arguments. 
	With flags that I defined as '#defines', we can know about the integrity of the input, which request to send to the server, and which memory to clean.
	The goal is to create a message from the received arguments, connect to an HTTP server, and receive a HTML code of the requested site.
	When a valid input is right, the information is saved in the structs: timeDeatils (contains information for -d) and address (contains login information).

## additional functions
* checkArgument: Check the integrity of the input in argv[i]
* checkUrl: Check the integrity of the URL input and create a struct with the URL details
* checkTime: Check the integrity of the time input and create a struct with the time details
* createMessage: Create a message with the login information written to the server via socket
* createSocket: Create socket and connecting to the server
* destroy: Destroys - deallocating memory
