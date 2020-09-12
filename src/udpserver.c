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
#include <time.h>

#include "pg1lib.h"

#define PORT    41026
#define MAX_LINE 4096

void usage(const char *program) {
    fprintf(stderr, "Usage: %s [PORT]\n", program);
}

int main(int argc, char *argv[]) {
    
    /* Variables */
    int port = PORT;
    int socket_fd = -1;
    struct sockaddr_in server_addr, remote_addr;
    int addrlen;
    int recvlen;
    char buffer[MAX_LINE];

    if (argc == 2) {
        port = atoi(argv[1]);
    } else {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    /* Generate server key */
    char* pubKey = getPubKey();

    /* Socket */
    if ((socket_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Unable to create socket: ");
        return EXIT_FAILURE;
    }

    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family  = AF_INET;
    server_addr.sin_addr.s_addr    = INADDR_ANY;
    server_addr.sin_port    = htons(port);

    /* Bind */
    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Unable to bind: ");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    while (1) {
        printf("Waiting...\n");

        /* Receive client key. Encrypt server key using client key. Send encrypted server key back to client. */
        char *client_key;
        char *encrypted_pubKey;
        addrlen = sizeof(remote_addr);
        while (1) {
            recvlen = recvfrom(socket_fd, buffer, MAX_LINE, 0, (struct sockaddr *)&remote_addr, (socklen_t *)&addrlen);
            if (recvlen > 0) {
                buffer[recvlen] = '\0';
                client_key = strdup(buffer);
                encrypted_pubKey = encrypt(pubKey, client_key);

                if (sendto(socket_fd, encrypted_pubKey, strlen(encrypted_pubKey) + 1, 0, (struct sockaddr *)&remote_addr, (socklen_t)sizeof(struct sockaddr)) < 0) {
                    fprintf(stderr, "Could not send encrypted server key.\n");
                    perror("sendto failed: ");
                    close(socket_fd);
                    return EXIT_FAILURE;
                }
                break;
            }
        }

        /* Receive encrypted message from client. Decrypt encrypted message. */
        char *encrypted_message;
        char *message;
        time_t time_received;
        while (1) {
            recvlen = recvfrom(socket_fd, buffer, MAX_LINE, 0, (struct sockaddr *)&remote_addr, (socklen_t *)&addrlen);
            if (recvlen > 0) {
                time_received = time(NULL);

                printf("------------ New Message ------------\n");
                printf("Received Time: %s\n", ctime(&time_received));
                printf("Beginning message:\n\n");

                buffer[recvlen] = '\0';
                encrypted_message = strdup(buffer);
                message = decrypt(encrypted_message);
                printf("%s\n\n", message);

                printf("End of message.\n\n");
                break;
            }
        }

        /* Receive checksum from client. Verify that calculated checksum matches sent checksum. */
        unsigned long client_checksum;
        unsigned long calculated_checksum;
        while (1) {
            recvlen = recvfrom(socket_fd, &client_checksum, sizeof(unsigned long), 0, (struct sockaddr *)&remote_addr, (socklen_t *)&addrlen);
            if (recvlen > 0) {
                printf("Received Client Checksum: %lu\n", client_checksum);

                calculated_checksum = checksum(message);
                printf("Calculated Checksum: %lu\n\n\n", calculated_checksum);
                break;
            }
        }

        /* Send back timestamp of when encrypted message was received back to client if checksum is valid */
        if (client_checksum == calculated_checksum) {
            if (sendto(socket_fd, &time_received, sizeof(time_t), 0, (struct sockaddr *)&remote_addr, (socklen_t)sizeof(struct sockaddr)) < 0) {
                fprintf(stderr, "Could not send received timestamp.\n");
                perror("sendto failed: ");
                close(socket_fd);
                return EXIT_FAILURE;
            }
        } else {
            printf("The checksums do not match.");
            long int error = 0;
            if (sendto(socket_fd, &error, sizeof(long int), 0, (struct sockaddr *)&remote_addr, (socklen_t)sizeof(struct sockaddr)) < 0) {
                fprintf(stderr, "Could not send error.\n");
                perror("sendto failed: ");
                close(socket_fd);
                return EXIT_FAILURE;
            }
        }
    }

    return 0; 
}