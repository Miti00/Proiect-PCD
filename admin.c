#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 7070
#define ADMIN_COMMAND "TERMINATE"

int main() {
  int inet_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (inet_socket < 0) {
    perror("Eroare la crearea socket-ului");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in server_address = {0};
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_address.sin_port = htons(SERVER_PORT);

  if (connect(inet_socket, (struct sockaddr *)&server_address,
              sizeof(server_address)) < 0) {
    perror("Nu a reusit conexiunea");
    exit(EXIT_FAILURE);
    printf("connected.\n");
  }

  // Send the termination command to the server
  if (send(inet_socket, ADMIN_COMMAND, strlen(ADMIN_COMMAND), 0) < 0) {
    perror("Error sending command to server");
    exit(1);
  }

  printf("Sent termination command to the server\n");

  close(inet_socket);

  return 0;
}