#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define UNIX_SOCKET_PATH "/tmp/unix_socket36"
#define MAX_BUFFER_SIZE 4096

int main() {
  int unix_socket;
  struct sockaddr_un unix_address;

  // Create Unix socket
  unix_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (unix_socket == -1) {
    perror("Error creating Unix socket");
    exit(1);
  }

  // Set up Unix socket address
  memset(&unix_address, 0, sizeof(unix_address));
  unix_address.sun_family = AF_UNIX;
  strncpy(unix_address.sun_path, UNIX_SOCKET_PATH,
          sizeof(unix_address.sun_path) - 1);

  // Connect to the Unix socket
  if (connect(unix_socket, (struct sockaddr *)&unix_address,
              sizeof(unix_address)) == -1) {
    perror("Error connecting to Unix socket");
    exit(1);
  }

  printf("Connected to Unix socket\n");

  char filename[100];
  printf("Introduceti numele fisierului BMP: ");
  fgets(filename, 100, stdin);
  filename[strcspn(filename, "\n")] = '\0'; // Eliminăm caracterul newline

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    perror("Eroare la deschiderea fisierului BMP");
    exit(EXIT_FAILURE);
  }

  // Trimite numele fisierului catre server
  send(unix_socket, filename, strlen(filename), 0);

  // Trimite continutul fisierului catre server
  char buffer[1024];
  size_t bytes_read;
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
    send(unix_socket, buffer, bytes_read, 0);
  }

  fclose(fp);

  printf("Fisierul a fost trimis cu succes catre server.\n");
  // Close the Unix socket
  close(unix_socket);

  return 0;
}