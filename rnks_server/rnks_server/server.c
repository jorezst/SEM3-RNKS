#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include "ack.h"
#include "packet.h"


#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable: 4996)


int main(int argc, char** argv) {
    WSADATA wsa;
    SOCKET server_socket;
    struct sockaddr_in6 server_address, client_address;
    struct ack ack;
    struct packet package;
    int client_address_len, n;
    ack.seqNr = 0;
    int PORT;
    FILE* file;
    char FILE_NAME[255];
  

    // Check if enough arguments
    if (!argv[1] || !argv[2]) {
        printf("Not enough parameters.\nPlease enter the name of the output file and the port number.");
        exit(EXIT_FAILURE);
    }

    // Copy first argument to FILE_NAME
    ZeroMemory(FILE_NAME, sizeof(FILE_NAME));
    strcat(FILE_NAME, argv[1]);

    // Copy second argument to PORT
    if (!(PORT = strtol(argv[2], NULL, 10))) {
        printf("Port invalid\n");
        exit(EXIT_FAILURE);
    }
    printf("Listening on Port: %d\n", PORT);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock. Error code: %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Winsock initialized\n");

    // Create a ipv6 socket
    server_socket = socket(AF_INET6, SOCK_DGRAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Error creating socket. Error code: %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Socket created\n");

    // Set server address, family and port number
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin6_family = AF_INET6;
    server_address.sin6_addr = in6addr_any;
    server_address.sin6_port = htons(PORT);

    // Open the text file to save the messages
    file = fopen(FILE_NAME, "w");
    if (file == NULL) {
        printf("Error opening output file\n");
        exit(EXIT_FAILURE);
    }
    printf("Opened output file\n");

    // Bind the socket to the server address
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
        printf("Error binding socket. Error code: %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Socket bound\n");
    printf("---------------------------------------\n\n");

    while (1) {
        // Receive data from client
        client_address_len = sizeof(client_address);
        n = recvfrom(server_socket, (char*)&package, sizeof(package), 0, (struct sockaddr*)&client_address, &client_address_len);
        if (n < 0) {
            printf("Error receiving data. Error code: %d", WSAGetLastError());
            exit(EXIT_FAILURE);
        }
        // Check if received seqNr is equal
        printf("Package SeqNr: %d \nack SeqNr %d\n", package.seqNr, ack.seqNr);
        if (package.seqNr == ack.seqNr) {
            // Print received data
            printf("SeqNr: %d \nReceived Data: %s", package.seqNr, package.txtCol);
            // Write line to output file
            fprintf(file, "%s", package.txtCol);

            // Send positive ack including seqNr
            n = sendto(server_socket, (char*)&ack, sizeof(ack), 0, (struct sockaddr*)&client_address, client_address_len);
            if (n < 0) {
                printf("Error sending receipt. Error code: %d", WSAGetLastError());
                exit(EXIT_FAILURE);
            }
            printf("Sent receipt\n\n");
            // Update seqNr
            ack.seqNr = (ack.seqNr + 1) % 2;
        }
        
        else {
            printf("Incorrect sequence number: %d\n", package.seqNr);
        }
    }

    // Close the file and the socket and cleanup
    fclose(file);
    closesocket(server_socket);
    WSACleanup();
    return 0;
}