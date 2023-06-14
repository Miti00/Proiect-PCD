#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 8080

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
  send(inet_socket, filename, strlen(filename), 0);

  // Trimite continutul fisierului catre server
  char buffer[1024];
  size_t bytes_read;
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
    send(inet_socket, buffer, bytes_read, 0);
  }

  fclose(fp);

  printf("Fisierul a fost trimis cu succes catre server.\n");

  // char txt_filename[4096];
  // ssize_t bytesRead;

  // // Receive the filename from the client
  // bytesRead = recv(inet_socket, txt_filename, strlen(txt_filename), 0);
  // if (bytesRead <= 0) {
  //   perror("Error receiving filename Inet");
  //   return 1;
  // }

  // // received_filename[bytesRead] = '\0';
  // printf("received txt filename: %s\n", txt_filename);

  // // Create the current filename
  // char received_txt_filename[4096];
  // sprintf(received_txt_filename, "received_%s", txt_filename);

  // Open the file for writing
  FILE *file = fopen("received_txt_filename.txt", "wb");
  if (file == NULL) {
    perror("Failed to create file Inet");
    return 1;
  }

  char txt_file[4096];
  ssize_t bytesRead;

  // Receive and save the BMP file
  while ((bytesRead = recv(inet_socket, txt_file, sizeof(txt_file), 0)) > 0) {
    if (fwrite(txt_file, 1, bytesRead, file))
      ;
    {
      perror("Error writing to file");
      exit(1);
    }
  }

  fclose(file);

  close(inet_socket);

  return 0;
}