#include "image.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define PORT 8080
#define PORT_ADMIN 7070
#define UNIX_SOCKET_PATH "/tmp/unix_socket36"
#define ADMIN_COMMAND "TERMINATE"

int main() {
  int inet_socket, client_inet_socket, unix_socket, admin_socket,
      client_admin_socket;
  struct sockaddr_in inet_address;
  int opt = 1;
  int inet_addrlen = sizeof(inet_address);
  struct sockaddr_un unix_address;
  struct sockaddr_in admin_address;
  int admin_addrlen = sizeof(inet_address);
  fd_set readFds;
  int maxFd;

  // Crearea socket-ului inet
  if ((inet_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Failed to create Inet socket");
    exit(EXIT_FAILURE);
  }
  // Setarea opțiunilor socket-ului inet
  if (setsockopt(inet_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("Failed to set Inet socket options");
    exit(EXIT_FAILURE);
  }
  inet_address.sin_family = AF_INET;
  inet_address.sin_addr.s_addr = INADDR_ANY;
  inet_address.sin_port = htons(PORT);

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

  // Crearea socket-ului admin
  if ((admin_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Failed to create socket");
    exit(EXIT_FAILURE);
  }
  // Setarea opțiunilor socket-ului admin
  if (setsockopt(admin_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("Failed to set socket options");
    exit(EXIT_FAILURE);
  }
  inet_address.sin_family = AF_INET;
  inet_address.sin_addr.s_addr = INADDR_ANY;
  inet_address.sin_port = htons(PORT_ADMIN);

  // Asocierea adresei IP și portului cu socket-ul inet
  if (bind(inet_socket, (struct sockaddr *)&inet_address,
           sizeof(inet_address)) < 0) {
    perror("Failed to bind Inet");
    exit(EXIT_FAILURE);
  }

  // Asocierea adresei IP și portului cu socket-ul unix
  if (bind(unix_socket, (struct sockaddr *)&unix_address,
           sizeof(unix_address)) == -1) {
    perror("Error binding Unix socket");
    exit(1);
  }

  // Asocierea adresei IP și portului cu socket-ul admin
  if (bind(admin_socket, (struct sockaddr *)&admin_address,
           sizeof(admin_address)) < 0) {
    perror("Failed to bind socket");
    exit(EXIT_FAILURE);
  }

  // Ascultarea conexiunilor la socket inet
  if (listen(inet_socket, 10) < 0) {
    perror("Failed to listen Inet ");
    exit(EXIT_FAILURE);
  }

  // Start listening for client connections on Unix socket
  if (listen(unix_socket, 10) < 0) {
    perror("Error listening on Unix socket");
    exit(1);
  }

  // Ascultarea conexiunilor la socket inet
  if (listen(admin_socket, 5) < 0) {
    perror("Failed to listen");
    exit(EXIT_FAILURE);
  }

  printf("Server is listening on INET port %d\n", PORT);
  printf("Server listening on Unix socket: %s\n", UNIX_SOCKET_PATH);
  //printf("Server is listening on admin port %d\n", PORT_ADMIN);

  // Initialize the file descriptor set and set the maxFd
  FD_ZERO(&readFds);
  FD_SET(unix_socket, &readFds);
  FD_SET(inet_socket, &readFds);
  FD_SET(admin_socket, &readFds);

  // Find the maximum file descriptor
  maxFd = unix_socket;
  if (inet_socket > maxFd) {
    maxFd = inet_socket;
  }
  if (admin_socket > maxFd) {
    maxFd = admin_socket;
  }

  while (1) {
    // Wait for activity on the sockets
    fd_set activeFds = readFds;
    if (select(maxFd + 1, &activeFds, NULL, NULL, NULL) < 0) {
      perror("Error in select");
      exit(1);
    }

    // check for activity on admin socket
    if (FD_ISSET(admin_socket, &activeFds)) {
      // Așteptarea unei conexiuni de la client Inet
      client_admin_socket =
          accept(admin_socket, (struct sockaddr *)&admin_address,
                 (socklen_t *)&admin_addrlen);
      if (client_admin_socket < 0) {
        perror("Failed to accept connection Inet");
        exit(EXIT_FAILURE);
      }
      printf("Client connected to Admin socket\n");
      char buffer[1024];
      ssize_t bytesRead;
      // Receive client request
      while ((bytesRead =
                  recv(client_admin_socket, buffer, sizeof(buffer), 0)) > 0) {
        // Check if the received command is the termination command
        if (strncmp(buffer, ADMIN_COMMAND, strlen(ADMIN_COMMAND)) == 0) {
          printf("Received termination command. Terminating server...\n");
          // Perform any necessary cleanup or shutdown operations
          unlink(UNIX_SOCKET_PATH);
          // Close client socket
          close(admin_socket);

          // Terminate the server
          exit(0);
        }
        // Clear the buffer
        memset(buffer, 0, sizeof(buffer));
      }
    }

    // Check for activity on Unix socket
    if (FD_ISSET(unix_socket, &activeFds)) {
      // Accept client connections on Unix socket
      int client_unix_socket = accept(unix_socket, NULL, NULL);
      if (client_unix_socket < 0) {
        perror("Error accepting client connection on Unix socket");
        exit(1);
      }

      printf("Client connected to Unix socket\n");

      char received_filename[4096];
      ssize_t bytesRead;

      // Receive the filename from the client
      bytesRead = recv(client_unix_socket, received_filename, 4096, 0);
      if (bytesRead <= 0) {
        perror("Error receiving filename Unix");
        return 1;
      }

      received_filename[bytesRead] = '\0';
      printf("Received filename Unix: %s\n", received_filename);

      // Truncate the last 4 characters from the received filename
      if (bytesRead >= 4) {
        received_filename[bytesRead - 4] = '\0';
      }

      // Set the seed for the random number generator
      srand(time(NULL));

      // Generate a random number between 0 and RAND_MAX
      int randomNumber = rand();

      // Create the current filename
      char current_filename[4096];
      sprintf(current_filename, "%s_%d.bmp", received_filename, randomNumber);

      // current_filename[bytesRead] = '\0';
      printf("Current filename Unix: %s\n", current_filename);

      // Open the file for writing
      FILE *file = fopen(current_filename, "wb");
      if (file == NULL) {
        perror("Failed to create file Unix");
        return 1;
      }

      // Receive and save the BMP file
      while ((bytesRead =
                  recv(client_inet_socket, received_filename, 4096, 0)) > 0) {
        fwrite(received_filename, 1, bytesRead, file);
      }

      fclose(file);

      FILE *fp = fopen(current_filename, "rb");
      if (fp == NULL)
        return NULL;
      struct BMP *bmp;
      bmp = (struct BMP *)malloc(sizeof(struct BMP));

      fread(bmp->header.name, 2, 1, fp);
      fread(&(bmp->header.size), 3 * sizeof(int), 1, fp);
      fread(&(bmp->dibheader), sizeof(struct DIB_header), 1, fp);

      fseek(fp, bmp->header.image_offset, SEEK_SET);
      bmp->image = readImage(fp, bmp->dibheader.height, bmp->dibheader.width);

      // Truncate the last 4 characters from the received filename
      char filename[4096];
      strcpy(filename, current_filename);
      size_t length = strlen(filename);
      if (length >= 4) {
        filename[length - 4] = '\0';
      }

      // Create the filename for .txt file
      char txt_filename[4096];
      sprintf(txt_filename, "%s.txt", filename);
      printf("txt filename: %s\n", txt_filename);

      // Create the filename for BW .bmp file
      char bw_filename[4096];
      sprintf(bw_filename, "%s_BW.bmp", filename);
      printf("BW filename: %s\n", bw_filename);

      imageToText(bmp->image, current_filename, txt_filename);

      BMPwriteBW(bmp, current_filename, bw_filename);

      fclose(fp);

      // Close client socket
      close(client_unix_socket);
    }

    // check for activity on Inet socket
    if (FD_ISSET(inet_socket, &activeFds)) {
      // Așteptarea unei conexiuni de la client Inet
      client_inet_socket = accept(inet_socket, (struct sockaddr *)&inet_address,
                                  (socklen_t *)&inet_addrlen);
      if (client_inet_socket < 0) {
        perror("Failed to accept connection Inet");
        exit(EXIT_FAILURE);
      }

      printf("Client connected to Inet socket\n");

      char received_filename[4096];
      ssize_t bytesRead;

      // Receive the filename from the client
      bytesRead = recv(client_inet_socket, received_filename, 4096, 0);
      if (bytesRead <= 0) {
        perror("Error receiving filename Inet");
        return 1;
      }

      // received_filename[bytesRead] = '\0';
      printf("Received filename Inet: %s\n", received_filename);

      // Truncate the last 4 characters from the received filename
      if (bytesRead >= 4) {
        received_filename[bytesRead - 4] = '\0';
      }

      // Create the current filename
      char current_filename[4096];
      sprintf(current_filename, "%s_%d.bmp", received_filename,
              ntohs(inet_address.sin_port));

      // current_filename[bytesRead] = '\0';
      printf("Current filename Inet: %s\n", current_filename);

      // Open the file for writing
      FILE *file = fopen(current_filename, "wb");
      if (file == NULL) {
        perror("Failed to create file Inet");
        return 1;
      }

      // Receive and save the BMP file
      while ((bytesRead =
                  recv(client_inet_socket, received_filename, 4096, 0)) > 0) {
        fwrite(received_filename, 1, bytesRead, file);
      }

      fclose(file);

      FILE *fp = fopen(current_filename, "rb");
      if (fp == NULL)
        return NULL;
      struct BMP *bmp;
      bmp = (struct BMP *)malloc(sizeof(struct BMP));

      fread(bmp->header.name, 2, 1, fp);
      fread(&(bmp->header.size), 3 * sizeof(int), 1, fp);
      fread(&(bmp->dibheader), sizeof(struct DIB_header), 1, fp);

      fseek(fp, bmp->header.image_offset, SEEK_SET);
      bmp->image = readImage(fp, bmp->dibheader.height, bmp->dibheader.width);

      // Truncate the last 4 characters from the received filename
      char filename[4096];
      strcpy(filename, current_filename);
      size_t length = strlen(filename);
      if (length >= 4) {
        filename[length - 4] = '\0';
      }

      // Create the filename for .txt file
      char txt_filename[4096];
      sprintf(txt_filename, "%s.txt", filename);
      printf("txt filename: %s\n", txt_filename);

      // Create the filename for BW .bmp file
      char bw_filename[4096];
      sprintf(bw_filename, "%s_BW.bmp", filename);
      printf("BW filename: %s\n", bw_filename);

      imageToText(bmp->image, current_filename, txt_filename);

      BMPwriteBW(bmp, current_filename, bw_filename);

      fclose(fp);

      // Open the txt file
      FILE *sendTxt = fopen(txt_filename, "rb");
      if (sendTxt == NULL) {
        perror("Error opening BMP file");
        exit(1);
      }

      // Trimite numele fisierului txt catre client inapoi
      // send(client_inet_socket, txt_filename, strlen(txt_filename), 0);

      // Read and send the txt file data
      char buffer[4096];
      size_t bytes_read;
      while ((bytes_read = fread(buffer, 1, sizeof(buffer), sendTxt)) > 0) {
        if (send(client_inet_socket, buffer, bytes_read, 0) == -1) {
          perror("Error sending file");
          exit(1);
        };
      }
      fclose(sendTxt);
      printf("txt file sent successfully\n");

      // Close the client socket
      close(client_inet_socket);
    }
  }
  return 0;
}

struct Image readImage(FILE *fp, int height, int width) {
  struct Image pic;
  int i, bytestoread, numOfrgb;
  pic.rgb = (struct RGB **)malloc(height * sizeof(void *));
  pic.height = height;
  pic.width = width;
  bytestoread = ((24 * width + 31) / 32) * 4;
  numOfrgb = bytestoread / sizeof(struct RGB) + 1;

  for (i = height - 1; i >= 0; i--) {
    // pic.rgb[i] = (struct RGB*) malloc(width*sizeof(struct RGB));
    pic.rgb[i] = (struct RGB *)malloc(numOfrgb * sizeof(struct RGB));
    fread(pic.rgb[i], 1, bytestoread, fp);
  }

  return pic;
}

void freeImage(struct Image pic) {
  int i;
  for (i = pic.height - 1; i >= 0; i--)
    free(pic.rgb[i]);
  free(pic.rgb);
}

unsigned char grayscale(struct RGB rgb) {
  return ((0.3 * rgb.red) + (0.6 * rgb.green) + (0.1 * rgb.blue));
  // return (rgb.red + rgb.green + rgb.blue)/3;
}

void RGBImageToGrayscale(struct Image pic) {
  int i, j;
  for (i = 0; i < pic.height; i++)
    for (j = 0; j < pic.width; j++)
      pic.rgb[i][j].red = pic.rgb[i][j].green = pic.rgb[i][j].blue =
          grayscale(pic.rgb[i][j]);
}

// TEXT Art
void imageToText(struct Image img, char *filename, char *txt_filename) {
  unsigned char gs;
  int i, j;
  //	char textpixel[] =
  //"@B%8&WM#ahdpmZOQLCJYXzcunxrjft{}|()1[]?-_+~<>i!I;:,
  //";
  char textpixel[] = "@&B8%W#Oao!:-,. ";
  // char textpixel[] = "@&#Oa-. ";
  int max = strlen(textpixel);
  int len = 256 / max;
  //	ahdpmZOQLCJYXzcunxrjft{}|()1[]?-_+~<>i!I;:,    ";
  //{'@', '#', '%', 'O', 'a', '-', '.', ' '}; 	// 0-31, 32-63, 64-95,
  // 96-127
  //...

  FILE *file = fopen(txt_filename, "w");
  if (file == NULL) {
    perror("Failed to open file");
    return 1;
  }

  for (i = 0; i < img.height; i += 2) {
    for (j = 0; j < img.width; j += 2) {
      gs = grayscale(img.rgb[i][j]);
      fprintf(file, "%c", textpixel[max - 1 - gs / len]);
    }
    fprintf(file, "\n");
  }

  fclose(file);

  /*for (i = 0; i < img.height; i += 2) {
    for (j = 0; j < img.width; j += 2) {
      gs = grayscale(img.rgb[i][j]);
      printf("%c", textpixel[max - 1 - gs / len]);
    }
    printf("\n");
  }*/
}

int BMPwriteBW(struct BMP *bmp, char *filename, char *bw_filename) {
  int i;

  FILE *fpw = fopen(bw_filename, "w");
  if (fpw == NULL)
    return 1;

  RGBImageToGrayscale(bmp->image);

  fwrite(bmp->header.name, 2, 1, fpw);
  fwrite(&(bmp->header.size), 3 * sizeof(int), 1, fpw);
  fwrite(&(bmp->dibheader), sizeof(struct DIB_header), 1, fpw);

  for (i = bmp->image.height - 1; i >= 0; i--)
    fwrite(bmp->image.rgb[i], ((24 * bmp->image.width + 31) / 32) * 4, 1, fpw);

  fclose(fpw);
  return 0;
}
