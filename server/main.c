#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>

#define PORT 9000
#define MAX_PACKET_SIZE 1024

int running = 1;

void handle_signal(int sig) { running = 0; }

void append_to_file(const char *filename, const char *data) {
  FILE *file = fopen(filename, "a");
  if (file != NULL) {
    fprintf(file, "%s\n", data);
    fclose(file);
  }
}

void process_client(int client_socket) {
  char buffer[MAX_PACKET_SIZE];
  ssize_t received;
  char client_address[INET_ADDRSTRLEN];

  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  getpeername(client_socket, (struct sockaddr *)&client_addr, &client_addr_len);
  inet_ntop(AF_INET, &(client_addr.sin_addr), client_address, INET_ADDRSTRLEN);

  syslog(LOG_INFO, "Accepted connection from %s", client_address);

  while (running) {
    received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (received > 0) {
      buffer[received] = '\0';
      append_to_file("/var/tmp/aesdsocketdata", buffer);
      send(client_socket, buffer, received, 0);
    } else if (received == 0) {

      break;
    } else {
      perror("Error receiving data");
      break;
    }
  }

  close(client_socket);
  syslog(LOG_INFO, "Closed connection from %s", client_address);
}

int main(int argc, char *argv[]) {
  int server_socket, client_socket;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_addr_len;
  pid_t pid;

  openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

  // Set up signal handling
  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);

  // Create server socket
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    perror("Error creating server socket");
    return -1;
  }

  // Set socket options
  int reuseaddr = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuseaddr,
                 sizeof(reuseaddr)) == -1) {
    perror("Error setting socket options");
    return -1;
  }

  // Bind socket to port
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);
  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) == -1) {
    perror("Error binding socket");
    return -1;
  }

  // Listen for connections

  if (listen(server_socket, 1) == -1) {
    perror("Error listening for connections");
    return -1;
  }

  // Daemonize the process if -d argument is provided

  if (argc > 1 && strcmp(argv[1], "-d") == 0) {
    pid = fork();

    if (pid == -1) {
      perror("Error forking process");
      return -1;
    } else if (pid > 0) {
      // Parent process exits
      return 0;
    }

    // Child process continues execution
    chdir("/");
    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Redirect standard file descriptors to /dev/null
    int dev_null = open("/dev/null", O_RDWR);
    dup2(dev_null, STDIN_FILENO);
    dup2(dev_null, STDOUT_FILENO);
    dup2(dev_null, STDERR_FILENO);
  }

  // Accept connections and process client requests
  while (running) {
    client_addr_len = sizeof(client_addr);
    client_socket = accept(server_socket, (struct sockaddr *)&client_addr,
                           &client_addr_len);

    if (client_socket == -1) {
      if (running)
        perror("Error accepting connection");
      continue;
    }

    pid = fork();
    if (pid == -1) {
      perror("Error forking process");
      return -1;
    } else if (pid == 0) {
      // Child process handles client request

      close(server_socket);
      process_client(client_socket);

      return 0;
    } else {
      // Parent process continues accepting connections
      close(client_socket);
    }
  }

  // Cleanup and exit
  close(server_socket);

  unlink("/var/tmp/aesdsocketdata");
  syslog(LOG_INFO, "Caught signal, exiting");
  closelog();

  return 0;
}
