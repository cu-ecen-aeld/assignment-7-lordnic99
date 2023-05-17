#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <complex.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

/* Signal */
bool signal_receive = false;
const char *write_path = "/var/tmp/aesdsocketdata";
int server_sock;

/*-----------------*/

void write_to_file(const char *path, const char *write_content) {
  int fd = open(path, O_WRONLY | O_APPEND);
  if (fd < 0) {
    perror("open");
    exit(1);
  }
  if (write(fd, write_content, strcspn(write_content, "\n") + 1) < 0) {
    perror("write");
    exit(1);
  }
  close(fd);
}

// char *read_from_file(const char *path) {
//   int fd = open(path, O_RDONLY);
//   if (fd < 1) {
//     perror("open");
//     exit(1);
//   }
//   int file_size = lseek(fd, 0, SEEK_END);
//   printf("file size: %d",file_size);
//   lseek(fd, 0, SEEK_SET);
//   buffer
//   read(fd, buffer, file_size);
//   printf("read: %s",buffer);
//   close(fd);
//   return &buffer[0];
// }

void create_file(const char *path) {
  int fd = creat(path, 0666);
  if (fd < 0) {
    perror("create");
    exit(1);
  }
  close(fd);
}

void signal_handler(int signo) {
  syslog(LOG_INFO, "Caught signal, exiting");
  close(server_sock);
  remove(write_path);
  exit(0);
}


int main(int argc, char *argv[]) {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  openlog(NULL, LOG_PID, LOG_USER);
  if (access(write_path, F_OK) < 0) {
    create_file(write_path);
  }
  const uint16_t PORT_NUMBER = 9000;
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0) {
    perror("sock");
    exit(1);
  }
  int optval = 1;
  setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  struct sockaddr_in server;
  struct sockaddr_in client;
  memset(&server, 0, sizeof(server));
  memset(&client, 0, sizeof(client));
  server.sin_port = htons(PORT_NUMBER);
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_family = AF_INET;
  if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("bind");
    exit(1);
  }
  pid_t child_proc;
  if (argc == 2 && (strcmp(argv[1], "-d") == 0)) {
    child_proc = fork();
    if (child_proc == 0) {
      listen(server_sock, 5);
      socklen_t client_len = (socklen_t)sizeof(client);
      while (1) {
        int client_socket =
            accept(server_sock, (struct sockaddr *)&client, &client_len);
        char *client_ip = (char *)malloc(INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &client.sin_addr.s_addr, client_ip, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "%s%s", "Accepted connection from ", client_ip);
        char buffer[1000000];
        memset(buffer, 0, sizeof(buffer));
        read(client_socket, buffer, sizeof(buffer));
        printf("server receive: %s", buffer);
        write_to_file(write_path, &buffer[0]);
        int fd = open(write_path, O_RDONLY);
        int file_size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);
        char read_buffer[file_size];
        memset(read_buffer, 0, file_size);
        read(fd, read_buffer, file_size);
        close(fd);
        send(client_socket, read_buffer, file_size, 0);
        syslog(LOG_INFO, "%s%s", "Closed connection from ", client_ip);
        close(client_socket);
        free(client_ip);
      }
    }
  } else {
    listen(server_sock, 5);
    socklen_t client_len = (socklen_t)sizeof(client);
    while (!signal_receive) {
      int client_socket =
          accept(server_sock, (struct sockaddr *)&client, &client_len);
      char *client_ip = (char *)malloc(INET_ADDRSTRLEN);
      inet_ntop(AF_INET, &client.sin_addr.s_addr, client_ip, INET_ADDRSTRLEN);
      syslog(LOG_INFO, "%s%s", "Accepted connection from ", client_ip);
      char buffer[1000000];
      memset(buffer, 0, sizeof(buffer));
      read(client_socket, buffer, sizeof(buffer));
      printf("server receive: %s", buffer);
      write_to_file(write_path, &buffer[0]);
      int fd = open(write_path, O_RDONLY);
      int file_size = lseek(fd, 0, SEEK_END);
      lseek(fd, 0, SEEK_SET);
      char read_buffer[file_size];
      memset(read_buffer, 0, file_size);
      read(fd, read_buffer, file_size);
      close(fd);
      send(client_socket, read_buffer, file_size, 0);
      syslog(LOG_INFO, "%s%s", "Closed connection from ", client_ip);
      close(client_socket);
      free(client_ip);
    }
  }
  return EXIT_SUCCESS;
}
