/**
 * Author:  Yeon Kim
 * NetID:   ykim22
 **/

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netdb.h>
#include <sys/time.h>

#include "pg1lib.h"
  
#define PORT    41026
#define MAX_LINE 4096
  
void usage(const char *program) {
    fprintf(stderr, "Usage: %s [HOST] [PORT] [FILEPATH/STRING]\n", program);
}

int main(int argc, char *argv[]) { 

    /* Variables */
    char *host  = "localhost";
    int port    = PORT;
    int socket_fd = -1;
    struct hostent *host_info;
    struct sockaddr_in server_addr;
    char buffer[MAX_LINE];
    int addrlen;
    int recvlen;
    char *message;
    char *content = NULL;
    size_t size;
    char *encrypted_message;
    unsigned long client_checksum;
    char *encrypted_server_key;
    char *server_key;
    struct timeval start, end;
    time_t server_time_received;
    char *c_time_string;
  
    /* Argument parsing */
    if (argc == 4) {
        host = argv[1];
        port = atoi(argv[2]);
        message = argv[3];
    } else {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    /* Generated client key */
    char *pubKey = getPubKey();

    /* Socket */
    if ((socket_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror("Unable to create socket: ");
        return EXIT_FAILURE;
    } 

    host_info = gethostbyname(host);
    if (!host_info) {
        fprintf(stderr, "Could not obtain address of %s\n", host);
        close(socket_fd);
        return EXIT_FAILURE;
    }

    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy(host_info->h_addr, (char *)&server_addr.sin_addr, host_info->h_length);
    server_addr.sin_port = htons(port);

    /* Send client public key */
    if (sendto(socket_fd, pubKey, strlen(pubKey) + 1, 0, (struct sockaddr *)&server_addr, (socklen_t)sizeof(struct sockaddr)) < 0) {
        fprintf(stderr, "Could not send public key.\n");   
        perror("sendto failed: ");
        close(socket_fd);
        return EXIT_FAILURE;
    }
    
    /* Received encrypted server key and decrypt using client private key*/
    printf("Waiting for server key...\n");
    addrlen = sizeof(server_addr);
    while (1) {
        recvlen = recvfrom(socket_fd, buffer, MAX_LINE, 0, (struct sockaddr *)&server_addr, (socklen_t *)&addrlen);
        if (recvlen > 0) {
            buffer[recvlen] = '\0';
            encrypted_server_key = strdup(buffer);
            server_key = decrypt(encrypted_server_key);
            break;
        }
    }

    /* Encrypt message using server public key and generate checksum */
    FILE *fp = fopen(message, "r");
    size = 0;
    if (fp != NULL) {
        fseek(fp, 0 , SEEK_END);
        size = ftell(fp);
        rewind(fp);
        content = malloc((size + 1) * sizeof(*content));
        fread(content, size, 1, fp);
        content[size] = '\0';

        encrypted_message = encrypt(content, server_key);
        client_checksum = checksum(content);

        fclose(fp);
    } else {
        encrypted_message = encrypt(message, server_key);
        client_checksum = checksum(message);
    }

    /* Send encrypted message and also log time */
    gettimeofday(&start, NULL);
    if (sendto(socket_fd, encrypted_message, strlen(encrypted_message) + 1, 0, (struct sockaddr *)&server_addr, (socklen_t)sizeof(struct sockaddr)) < 0) {
        fprintf(stderr, "Could not send encrypted message.\n");
        perror("sendto failed: ");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    /* Send checksum */
    if (sendto(socket_fd, &client_checksum, sizeof(unsigned long), 0, (struct sockaddr *)&server_addr, (socklen_t)sizeof(struct sockaddr)) < 0) {
        fprintf(stderr, "Could not send checksum.\n");
        perror("sendto failed: ");
        close(socket_fd);
        return EXIT_FAILURE;
    }
    printf("Checksum sent: %lu\n", client_checksum);

    /* Receive timestamp from server and also log time */
    while (1) {
        recvlen = recvfrom(socket_fd, &server_time_received, sizeof(time_t), 0, (struct sockaddr *)&server_addr, (socklen_t *)&addrlen);
        if (recvlen > 0) {
            gettimeofday(&end, NULL);
            c_time_string = ctime(&server_time_received);
            if (!c_time_string) {
                fprintf(stderr, "Timestamp is invalid.\n");
                close(socket_fd);
                return EXIT_FAILURE;
            }
            printf("Server successfully received message at:\n %s\n", c_time_string);
            break;
        }
    }

    /* Calculate round-trip time */
    printf("Round-Trip Time: %ld usec\n", ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));

    close(socket_fd);
    return 0; 
}