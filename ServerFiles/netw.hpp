#include <stdio.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#define PORT 80 // The port users will be connecting to
#define WEBROOT "./webroot" // The webserver's root directory

void fatal(const char * message){
    std::cout<<"Error: "<< message << std::endl;
    exit(-1);
}

/* This function accepts an open file descriptor and returns
* the size of the associated file. Returns -1 on failure.
*/
int get_file_size(std::fstream & fd) {
    fd.seekg(0,std::ios::end);
    /*if(fstat(fd, &stat_struct) == -1)
        return -1;*/
    int length = fd.tellg();
    return length;
}

void dump(const unsigned char *data_buffer, const unsigned int length) {
    unsigned char byte;
    unsigned int i, j;
    for(i=0; i < length; i++) {
        byte = data_buffer[i];
        printf("%02x ", data_buffer[i]); // Display byte in hex.
        if(((i%16)==15) || (i==length-1)) {
            for(j=0; j < 15-(i%16); j++){
                printf(" ");
            }
            printf("| ");
            for(j=(i-(i%16)); j <= i; j++) { // Display printable bytes from line.
                byte = data_buffer[j];
                if((byte > 31) && (byte < 127)) // Outside printable char range
                    printf("%c", byte);
                else
                    printf(".");
            }
            printf("\n"); // End of the dump line (each line is 16 bytes)
        } // End if
    } // End for
}

/* This function handles the connection on the passed socket from the
* passed client address. The connection is processed as a web request,
* and this function replies over the connected socket. Finally, the
* passed socket is closed at the end of the function.
*/

/* This function accepts a socket FD and a ptr to the null terminated
* string to send. The function will make sure all the bytes of the
* string are sent. Returns 1 on success and 0 on failure.
*/
int send_string(int sockfd,const char *buffer) {
    int sent_bytes, bytes_to_send;
    bytes_to_send = strlen(buffer);
    while(bytes_to_send > 0) {
        sent_bytes = send(sockfd, buffer, bytes_to_send, 0);
        if(sent_bytes == -1)
            return 0; // Return 0 on send error.
        bytes_to_send -= sent_bytes;
        buffer += sent_bytes;
    }
    return 1; // Return 1 on success.
}

int recv_line(int sockfd, char *dest_buffer) {
    #define EOL "\r\n" // End-of-line byte sequence
    #define EOL_SIZE 2
    char *ptr;
    int eol_matched = 0;
    ptr = dest_buffer;
    while(recv(sockfd, (void*)ptr, 1, 0) == 1) { // Read a single byte.
        if(*ptr == EOL[eol_matched]) { // Does this byte match terminator?
            eol_matched++;
            if(eol_matched == EOL_SIZE) { // If all bytes match terminator,
                *(ptr+1-EOL_SIZE) = '\0'; // terminate the string.
                return strlen(dest_buffer); // Return bytes received
            }
        } 
        else {
            eol_matched = 0;
        }
        ptr++; // Increment the pointer to the next byter.
    }
    return 0; // Didn't find the end-of-line characters.
}

void handle_connection(int sockfd, struct sockaddr_in *client_addr_ptr) {
    char *ptr, request[500], resource[500];
    int length;
    std::fstream fd;
    length = recv_line(sockfd, request);
    printf("Got request from %s:%d \"%s\"\n", inet_ntoa(client_addr_ptr->sin_addr),ntohs(client_addr_ptr->sin_port), request);
    ptr = strstr(request, " HTTP/"); // Search for valid-looking request.
    if(ptr == NULL) { // Then this isn't valid HTTP.
        printf(" NOT HTTP!\n");
    } 
    else {
        *ptr = 0; // Terminate the buffer at the end of the URL.
        ptr = NULL; // Set ptr to NULL (used to flag for an invalid request).
        if(strncmp(request, "GET ", 4) == 0) // GET request
            ptr = request+4; // ptr is the URL.
        if(strncmp(request, "HEAD ", 5) == 0) // HEAD request
            ptr = request+5; // ptr is the URL.
        if(ptr == NULL) { // Then this is not a recognized request.
            printf("\tUNKNOWN REQUEST!\n");
        } 
        else { // Valid request, with ptr pointing to the resource name
            if (ptr[strlen(ptr) - 1] == '/') // For resources ending with '/',
                strcat(ptr, "index.html"); // add 'index.html' to the end.
            strcpy(resource, WEBROOT); // Begin resource with web root path
            strcat(resource, ptr); // and join it with resource path.
            fd.open((const char*)resource, std::fstream::in); // Try to open the file.
            length = get_file_size(fd);
            printf("\tOpening \'%s\'\t", resource);
            if(!fd) { // If file is not found
                printf(" 404 Not Found\n");
                send_string(sockfd, "HTTP/1.0 404 NOT FOUND\r\n");
                send_string(sockfd, "Server: Tiny webserver\r\n\r\n");
                send_string(sockfd, "<html><head><title>404 Not Found</title></head>");
                send_string(sockfd, "<body><h1>URL not found</h1></body></html>\r\n");
            } 
            else { // Otherwise, serve up the file.
                printf(" 200 OK\n");
                send_string(sockfd, "HTTP/1.0 200 OK\r\n");
                send_string(sockfd, "Server: Tiny webserver\r\n\r\n");
                if(ptr == request + 4) { // Then this is a GET request
                    if( (length = get_file_size(fd)) == -1)
                        fatal("getting resource file size");
                    if( (ptr = (char *) malloc(length)) == NULL)
                        fatal("allocating memory for reading resource");
                    fd.read( ptr, length); // Read the file into memory.
                    send(sockfd, ptr, length, 0); // Send it to socket.

                    free(ptr); // Free file memory.
                }
                fd.close(); // Close the file.
            } // End if block for file found/not found.
        } // End if block for valid request.
    } // End if block for valid HTTP.
    shutdown(sockfd, SHUT_RDWR); // Close the socket gracefully.
}