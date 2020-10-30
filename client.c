#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <time.h>
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define MAX_ARGC 5
#define MIN_ARGC 2
#define FLAG_URL flags[0]
#define FLAG_H flags[1]
#define FLAG_D flags[2]
#define INDEX_URL flags[3]
#define INDEX_TIME flags[4]
#define EXIT_FAILURE 1
#define VERSION " HTTP/1.1\r\nHost: "
#define conClose "\r\nConnection: close"
#define modif "\r\nIf-Modified-Since: "
#define EndRequest  "\r\n\r\n"
//Structs
typedef struct address{
	int port;  //the number of port
	char* host; //host name
	char* path; //path
}address;
typedef struct timeDetails{
	int day;
	int hour;
	int minute;
}timeDetails;
//DECLERATION OF FUNCTIONS
void checkArgument(char** argv, int argc, int* flags, int* i);
address* checkUrl(char* text);
timeDetails* checkTime(char* text, address* add);
char* createMessage(address*,timeDetails*,int* flags);
void createSocket(address* add,timeDetails* timeD_, int* flags, char* message);
void destroy(char* message, address* add,timeDetails* timeD_, int* flags);
void destroy_add(address* add);
void copySubString(char* dst, char* src, int start, int end);
int indexOf(char c, char* str, int start);
void destAdd_badUsage(address* add);
void destroyAndNull (void* x);
void destroy_error(char* message, address* add,timeDetails* timeD_,int* sockfd, int* flags, char* err);
int isNumber(char* str);
int numOfDigit(int x);
void error (char* err);
void badUsage();
/*****MAIN****/
int main(int argc, char *argv[]){
	if (argc > MAX_ARGC || argc < MIN_ARGC)
		badUsage();
	int i,flags[5] = {0,0,0,0,0};
	for (i=1; i<argc; i++)// check argv input
		checkArgument(argv, argc, flags,&i);
	if (FLAG_URL==0)// in case there is no URL in argv
		badUsage();
	timeDetails* timeD_;
	address* add = checkUrl(argv[INDEX_URL]);
	if(FLAG_D)// in case there is '-d'
		timeD_ = checkTime(argv[INDEX_TIME], add);
	char* message = createMessage(add,timeD_,flags); // Create a message for the socket
	createSocket(add, timeD_, flags, message);// Create a socket and send the message
	destroy(message,add, timeD_, flags);//Deallocate allocated memory
	return 0;
}
/* Check the integrity of the input in argv[i] */
void checkArgument(char** argv, int argc, int* flags, int* i){
	if (strcmp(argv[*i], "-h")==0)//argv[i] contain '-h'
		FLAG_H++;
	else if (strcmp(argv[*i], "-d")==0){ //argv[i] contain '-d'
		FLAG_D++;
		if (*i==argc-1)
			badUsage();
		INDEX_TIME = ++(*i);//argv[i+1] may contain time format
	}
	else if (strlen(argv[*i])>8){ //argv[i] may be URL in format http://*/
		FLAG_URL++;
		INDEX_URL = *i;
	}
	else
		badUsage();
	if (FLAG_D>1 || FLAG_H>1 || FLAG_URL>1) // If there are more than one
		badUsage();
}
/* Create a message with the login information written to the server via socket */
char* createMessage(address* add ,timeDetails* timeD_,int* flags){
	int length=6;// default message start length
	if (FLAG_H == 1)
		length++;
	char start [length];//Contains the beginning of the request
	if (FLAG_H == 1)
		copySubString(start, "HEAD /", -1,length);
	else
		copySubString(start, "GET /", -1,length);
	int size = numOfDigit(add->port);
	char portstr[size+1]; // contains array of chars with port number
	sprintf(portstr, ":%d", add->port);
	char timebuf[128];
	int len = strlen(start)+strlen(add->path)+strlen(VERSION)+strlen(add->host)+strlen(conClose)+strlen(EndRequest);
	if (add->port != 80) // calculate the length of the message
		len+= strlen(portstr);
	time_t 	now = time(NULL);
	if(FLAG_D == 1){ // in case ther is -d
		now=now-(timeD_->day*24*3600+timeD_->hour*3600+timeD_->minute*60);
		strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
		len+= strlen(modif) + strlen(timebuf);
	}
	char* message = (char*)malloc(len*sizeof(char)+1); // allocated message
	if (message == NULL)
		destroy_error(NULL, add, timeD_,NULL, flags, "malloc");
	strcpy(message,start);// connecting strings to the 'message'
	strcat(message, add->path);
	strcat(message, VERSION);
	strcat(message, add->host);
	if (add->port != 80)
	strcat(message, portstr);
	strcat(message, conClose);
	if(FLAG_D==1){
		strcat(message, modif);
		strcat(message, timebuf);
	}
	strcat(message, EndRequest);
	message[len] = '\0';
	printf("HTTP request =\n%s\nLEN = %d\n", message, len);
	return message;
}
/* Check the integrity of the URL input and create a struct with the URL details */
address* checkUrl(char* text){
	char http[7]; // 7 is length of 'http://'
	copySubString(http, text, -1,7);
	if (strcmp(http,"http://\0") != 0)// check if start with "http://"
		badUsage();
	int slash = indexOf('/', text, 7);//after "http://"
	int colon = indexOf(':', text, 7);
	if(slash == colon+1 || colon>slash) // :/ or /__:
		badUsage();
	int hostlen; // contain the length of 'host'
	if (slash == -1)// if we dont have "/"
		badUsage();
	if (colon == -1)// if we dont have ":"
		hostlen = slash-6;
	else
		hostlen = colon-6;
	address* add = (address*)malloc(sizeof(address)); // allocate struct address
	if (add == NULL)
		error("malloc\n");
	if ( (add->host = (char*)malloc(hostlen*sizeof(char))) == NULL){ // allocate host
		destroyAndNull(add);
		error("malloc\n");
	}
	if ((add->path = (char*)malloc((strlen(text)-slash)*sizeof(char))) == NULL){ // allocate path
		destroyAndNull (add->host);
		destroyAndNull (add);
		error("malloc\n");
	}
	add->port = 80; // deafult case
	if (colon!=-1)
		copySubString (add->host,text, 6, colon);// save host in add struct
	else
		copySubString(add->host, text, 6, slash);
	if(colon != -1){ //if we have ":" create integer with port number
		char portString[slash-colon-1]; 
		copySubString (portString,text, colon, slash);
		if (!isNumber(portString))
			destAdd_badUsage(add);
		add->port = atoi(portString);
		if (add->port <= 0)
			destAdd_badUsage(add);
	}
	copySubString(add->path, text, slash, strlen(text));// save path in add struct
	return add;
}
/* Check the integrity of the time input and create a struct with the time details */
timeDetails* checkTime(char* text, address* add){
	int first_colon = indexOf(':', text, 0); // index of first ':'
	if (first_colon == -1|| first_colon == 0)
		destAdd_badUsage(add);
	int second_colon = indexOf(':', text, first_colon+1); // index of second ':'
	if (second_colon == -1 || second_colon == strlen(text)-1|| second_colon==first_colon+1)
		destAdd_badUsage(add);
	if (indexOf(':', text, second_colon+1) != -1) //in case there is too much ':'
		destAdd_badUsage(add);
	char first [first_colon], second[second_colon-first_colon], third[strlen(text)-second_colon];
	copySubString(first, text, -1, first_colon);// contain day number
	copySubString(second, text, first_colon, second_colon);// contain hour number
	copySubString(third, text, second_colon, strlen(text));// contain minute number
	if (!isNumber(first) || !isNumber(second)|| !isNumber(third))
		destAdd_badUsage(add); // in case one of the variables is not a number
	timeDetails* timeD_ = (timeDetails*)malloc(sizeof(timeDetails));// allocate struct timeDetails
	if (timeD_ == NULL){
		destroy_add(add);
		error("malloc\n");
	}
	timeD_->day = atoi(first);
	timeD_->hour = atoi(second);
	timeD_->minute = atoi(third);
	return timeD_;
}
/* create socket and connecting to the server */
void createSocket(address* add,timeDetails* timeD_, int* flags, char* message){
	int sockfd,rc=1,n=1;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	if ((sockfd = socket(AF_INET, SOCK_STREAM,0)) < 0 )//create an endpoint for communication
		destroy_error(message, add, timeD_, NULL, flags, "ERROR opening socket\n");
	if ((server = gethostbyname(add->host)) == NULL){ // get server address
		destroy(message, add, timeD_, flags);
		herror("gethostbyname");
		if (close(sockfd) < 0 )
			perror("close");
		exit(EXIT_FAILURE);
	}
	serv_addr.sin_family = AF_INET; //initialize struct socaddr_in
	bcopy((char *)server ->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(add->port); // convert port to number
	if (connect(sockfd, (const struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) // connect
		destroy_error(message, add, timeD_, &sockfd, flags, "ERROR connecting\n");
	if ((rc = write(sockfd, message,strlen(message))) < 0)//write
		destroy_error(message, add, timeD_, &sockfd, flags, "ERROR write\n");
	//size_t size = 1024;
	unsigned char buffer [1024];
	bzero(buffer,1024);
	int count=0;
	while (n>0){
		if ((n = read(sockfd,buffer,sizeof(buffer))) < 0)// read
			destroy_error(message, add, timeD_, &sockfd, flags, "ERROR reading from socket\n");
		count+=n;
		if (n!= 0){
 			write(1,buffer,n);
			fflush(stdout);
		}
	}
	printf("\n Total received response bytes: %d\n",count);
	if (close(sockfd) < 0 )
		destroy_error(message, add, timeD_, &sockfd, flags, "close\n");
}
/*count num of digits in number*/
int numOfDigit(int x){
	int i=0;
	for(; x!=0; i++, x/=10);
	return i;
}
/* Check if String is a number - return 1 with success, 0 if fail*/
int isNumber(char* str){
	int i=0;
	if (*str=='-')
		i=1;
	for (; i<strlen(str); i++)// check if str[i] is a number. only integer
		if ( *(str+i) > '9' || *(str+i) <'0'|| i==8)
			return 0;
	return 1;
}
/* copy substring from src to dst between the index start-end*/
void copySubString(char* dst, char* src, int start, int end){
		memcpy(dst, src+start+1, end-start-1);
		dst[end-start-1] = '\0';
}
/* find index of specific char in string - return  -1 if fail*/
int indexOf(char c, char* str, int start){
	int i = start;
	for (; i<strlen(str); i++)
		if (*(str+i)==c)
			return i;
	return -1;
}
/* Destroys - deallocating memory. */
void destroy(char* message, address* add,timeDetails* timeD_, int* flags){
	if (FLAG_URL== 1 && add != NULL)
		destroy_add(add);
	if (FLAG_D == 1 && timeD_ != NULL)
		destroyAndNull (timeD_);
	if (message != NULL)
		destroyAndNull (message);
}/* Destroys address struct */
void destroy_add(address* add){
		destroyAndNull (add->host);
		destroyAndNull (add->path);
		destroyAndNull(add);
}
/* show an error meesage and exit(EXIT_FAILURE) */
void badUsage(){
	fprintf(stderr,"usage: client [–h] [–d <time-interval>] <URL>\n");
	exit(EXIT_FAILURE);
}
/* Destroys address struct and show an error meesage and exit(EXIT_FAILURE) */
void destAdd_badUsage(address* add){
		destroy_add(add);
		badUsage();
}
/* deallocating all memory, show an error message, closing socket and exit(EXIT_FAILURE) */
void destroy_error(char* message, address* add,timeDetails* timeD_, int* sockfd, int* flags, char* err){
	destroy (message, add, timeD_, flags);
	if (sockfd != NULL)
		if (close(*sockfd) < 0 ) // close socket
			perror("close\n");
	error(err);
}
/* show an error meesage and exit(EXIT_FAILURE) */
void error(char* err){
	perror(err);
	exit(EXIT_FAILURE);
}
/* Destroys and set as Null */
void destroyAndNull (void* x){
	free(x);
	x = NULL;
}
