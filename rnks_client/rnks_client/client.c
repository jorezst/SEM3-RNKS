#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include "packet.h"
#include "ack.h"

#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable: 4996)



int main(int argc, char** argv) {
    WSADATA wsa;
    SOCKET client_socket;
    struct sockaddr_in6 server_address;
    struct packet package;
    struct timeval timeout;
    struct ack ack;
    char buffer[BUFFER_SIZE];
    int PORT;
    int n;
    u_long iMode = 0;
    ack.seqNr = 0;
    FILE* file;
    int server_address_len;
    char SERVER_IP[40];
    char FILE_NAME[255];


    // Check if enough arguments
    if (!argv[1] || !argv[2] || !argv[3]) {
        printf("Not enough parameters.\nPlease enter the name of the file, the IPv6-Adress and the port number.");
        exit(EXIT_FAILURE);
    }

    // Copy first argument to FILE_NAME
    ZeroMemory(FILE_NAME, sizeof(FILE_NAME));
    strcat(FILE_NAME, argv[1]);

    // Copy second argument to SERVER_IP
    ZeroMemory(SERVER_IP, sizeof(SERVER_IP));
    strcat(SERVER_IP, argv[2]);

    // Copy third argument to PORT
    if (!(PORT = strtol(argv[3], NULL, 10))) {
        printf("Port invalid\n");
        exit(EXIT_FAILURE);
    }
    printf("Port: %d\n", PORT);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock. Error code: %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Winsock initialized\n");

    // Create a socket
    client_socket = socket(AF_INET6, SOCK_DGRAM, 0);
    if (client_socket == INVALID_SOCKET) {
        printf("Error creating socket. Error code: %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Socket created\n");

    // Set socket to nonblocking
    n = ioctlsocket(client_socket, FIONBIO, &iMode);
    if (n != NO_ERROR) {
        printf("ioctlsocket failed. Error code: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Set Socket to nonblocking\n");

    // Set the timeout value for the Socket
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
        printf("Error setting timeout. Error code: %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Set timeout value to 5 seconds\n");

    // Set server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin6_family = AF_INET6;
    inet_pton(AF_INET6, SERVER_IP, &server_address.sin6_addr);
    server_address.sin6_port = htons(PORT);

    // Open the text file
    file = fopen(FILE_NAME, "r");
    if (file == NULL) {
        printf("Error opening input file\n");
        exit(EXIT_FAILURE);
    }
    printf("Opened input file successfully\n");
    printf("---------------------------------------\n\n");

    // Read file liny by line
    while (fgets(package.txtCol, BUFFER_SIZE, file)) {

        // Set package.seqNr to ack.seqNr
        package.seqNr = ack.seqNr;
        while (1) {
            // Send data to server including package.seqNr
            n = sendto(client_socket, (char*)&package, sizeof(package), 0, (struct sockaddr*)&server_address, sizeof(server_address));
            if (n < 0) {
                printf("Error sending data. Error code: %d", WSAGetLastError());
                exit(EXIT_FAILURE);
            }
            // Setting timer to timeout after 5 seconds
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(client_socket, &readfds);
            n = select(client_socket + 1, &readfds, NULL, NULL, &timeout);
            if (n == 0) {
                // Timeout occurred
                printf("Timed out, resending data\n");
            }
            else if (n > 0) {
                // Wait for ack response
                n = recvfrom(client_socket, (char*)&ack, sizeof(ack), 0, NULL, NULL);
                if (n < 0) {
                    printf("Error receiving ACK. Error code: %d", WSAGetLastError());
                    exit(EXIT_FAILURE);
                }
                // Check if received seqNr is the same as the one sent
                if (package.seqNr == ack.seqNr) {
                    //Update seqNr
                    ack.seqNr = (ack.seqNr + 1) % 2;
                    break;
                }

            }

        }
    }


    // Close the file and the socket and cleanup
    fclose(file);
    closesocket(client_socket);
    WSACleanup();
    return 0;
}