//
// Created by wangxinshuo on 2020/11/7.
//

#ifndef RDMA_TEST_INCLUDE_RDMA_SOCKET_H_
#define RDMA_TEST_INCLUDE_RDMA_SOCKET_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>

#include <sys/time.h>
#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <sys/socket.h>
#include <netdb.h>

/******************************************************************************
* Function: sock_connect
*
* Input
* servername URL of server to connect to (NULL for server mode)
* port port of service
*
* Output
* none
*
* Returns
* socket (fd) on success, negative error code on failure
*
* Description
* Connect a socket. If servername is specified a client connection will be
* initiated to the indicated server and port. Otherwise listen on the
* indicated port for an incoming connection.
*
******************************************************************************/
static int sock_connect(const char *servername, int port) {
  struct addrinfo *resolved_addr = NULL;
  struct addrinfo *iterator;
  char service[6];
  int sockfd = -1;
  int listenfd = 0;
  int tmp;
  struct addrinfo hints =
      {
          .ai_flags = AI_PASSIVE,
          .ai_family = AF_INET,
          .ai_socktype = SOCK_STREAM};
  if (sprintf(service, "%d", port) < 0)
    goto sock_connect_exit;
  /* Resolve DNS address, use sockfd as temp storage */
  sockfd = getaddrinfo(servername, service, &hints, &resolved_addr);
  if (sockfd < 0) {
    fprintf(stderr, "%s for %s:%d\n", gai_strerror(sockfd), servername, port);
    goto sock_connect_exit;
  }
  /* Search through results and find the one we want */
  for (iterator = resolved_addr; iterator; iterator = iterator->ai_next) {
    sockfd = socket(iterator->ai_family, iterator->ai_socktype, iterator->ai_protocol);
    if (sockfd >= 0) {
      if (servername) {
        /* Client mode. Initiate connection to remote */
        if ((tmp = connect(sockfd, iterator->ai_addr, iterator->ai_addrlen))) {
          fprintf(stdout, "failed connect \n");
          close(sockfd);
          sockfd = -1;
        }
      } else {
        /* Server mode. Set up listening socket an accept a connection */
        listenfd = sockfd;
        sockfd = -1;
        if (bind(listenfd, iterator->ai_addr, iterator->ai_addrlen))
          goto sock_connect_exit;
        listen(listenfd, 1);
        sockfd = accept(listenfd, NULL, 0);
      }
    }
  }
  sock_connect_exit:
  if (listenfd)
    close(listenfd);
  if (resolved_addr)
    freeaddrinfo(resolved_addr);
  if (sockfd < 0) {
    if (servername)
      fprintf(stderr, "Couldn't connect to %s:%d\n", servername, port);
    else {
      perror("server accept");
      fprintf(stderr, "accept() failed\n");
    }
  }
  return sockfd;
}

/******************************************************************************
* Function: sock_sync_data
*
* Input
* sock socket to transfer data on
* xfer_size size of data to transfer
* local_data pointer to data to be sent to remote
*
* Output
* remote_data pointer to buffer to receive remote data
*
* Returns
* 0 on success, negative error code on failure
*
* Description
* Sync data across a socket. The indicated local data will be sent to the
* remote. It will then wait for the remote to send its data back. It is
* assumed that the two sides are in sync and call this function in the proper
* order. Chaos will ensue if they are not. :)
*
* Also note this is a blocking function and will wait for the full data to be
* received from the remote.
*
******************************************************************************/
int sock_sync_data(int sock, int xfer_size, char *local_data, char *remote_data) {
  int rc;
  int read_bytes = 0;
  int total_read_bytes = 0;
  rc = write(sock, local_data, xfer_size);
  if (rc < xfer_size) {
    fprintf(stderr, "Failed writing data during sock_sync_data\n");
  } else {
    rc = 0;
  }
  while (!rc && total_read_bytes < xfer_size) {
    read_bytes = read(sock, remote_data, xfer_size);
    if (read_bytes > 0) {
      total_read_bytes += read_bytes;
    } else {
      rc = read_bytes;
    }
  }
  return rc;
}

#endif //RDMA_TEST_INCLUDE_RDMA_SOCKET_H_
