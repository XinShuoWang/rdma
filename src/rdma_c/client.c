/*
* BUILD COMMAND:
* gcc -Wall -I/usr/local/ofed/include -O2 -o RDMA_RC_example -L/usr/local/ofed/lib64 -L/usr/local/ofed/lib -
libverbs RDMA_RC_example.c
*
*/
/******************************************************************************
*
* RDMA Aware Networks Programming Example
*
* This code demonstrates how to perform the following operations using the * VPI Verbs API:
*
* Send
* Receive
* RDMA Read
* RDMA Write
*
*****************************************************************************/

#include "rdma_predefine.h"
#include "rdma_structure.h"
#include "rdma_resource.h"
#include "rdma_helper.h"
#include "rdma_poll.h"
#include "rdma_connect.h"

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

struct config_t config = {
    NULL,  /* dev_name */
    NULL,  /* server_name */
    19875, /* tcp_port */
    1,       /* ib_port */
    -1 /* gid_idx */
};

/******************************************************************************
* Function: main
*
* Input
* argc number of items in argv
* argv command line parameters
*
* Output
* none
*
* Returns
* 0 on success, 1 on failure
*
* Description
* Main program code
******************************************************************************/
int main(int argc, char *argv[]) {
  struct resources res;
  int rc = 1;
  char temp_char;

  config.server_name = "10.11.6.132";
  config.tcp_port = 19875;

  printf("servername=%s\n", config.server_name);
  /* print the used parameters for info*/
  print_config(&config);
  /* init all of the resources, so cleanup will be easy */
  resources_init(&res);
  /* create resources before using them */
  if (resources_create(&res, &config)) {
    fprintf(stderr, "failed to create resources\n");
    goto main_exit;
  }
  /* connect the QPs */
  if (connect_qp(&res, &config)) {
    fprintf(stderr, "failed to connect QPs\n");
    goto main_exit;
  }

  /* in both sides we expect to get a completion */
  if (poll_completion(&res)) {
    fprintf(stderr, "poll completion failed\n");
    goto main_exit;
  }

  /* after polling the completion we have the message in the client buffer too */
  fprintf(stdout, "Message is: '%s'\n", res.buf);

  /* Sync so we are sure server side has data ready before client tries to read it */
  /* just send a dummy char back and forth */
  if (sock_sync_data(res.sock, 1, "R", &temp_char)) {
    fprintf(stderr, "sync error before RDMA ops\n");
    rc = 1;
    goto main_exit;
  }
  // Now the client performs an RDMA read and then write on server.
  // Note that the server has no idea these events have occured
  /* First we read contens of server's buffer */
  if (post_send(&res, IBV_WR_RDMA_READ)) {
    fprintf(stderr, "failed to post SR 2\n");
    rc = 1;
    goto main_exit;
  }
  if (poll_completion(&res)) {
    fprintf(stderr, "poll completion failed 2\n");
    rc = 1;
    goto main_exit;
  }
  fprintf(stdout, "Contents of server's buffer: '%s'\n", res.buf);

  /* Now we replace what's in the server's buffer */
  strcpy(res.buf, RDMAMSGW);

  fprintf(stdout, "Now replacing it with: '%s'\n", res.buf);
  if (post_send(&res, IBV_WR_RDMA_WRITE)) {
    fprintf(stderr, "failed to post SR 3\n");
    rc = 1;
    goto main_exit;
  }
  if (poll_completion(&res)) {
    fprintf(stderr, "poll completion failed 3\n");
    rc = 1;
    goto main_exit;
  }

  /* Sync so server will know that client is done mucking with its memory */
  /* just send a dummy char back and forth */
  if (sock_sync_data(res.sock, 1, "W", &temp_char)) {
    fprintf(stderr, "sync error after RDMA ops\n");
    rc = 1;
    goto main_exit;
  }
  rc = 0;

  main_exit:
  if (resources_destroy(&res)) {
    fprintf(stderr, "failed to destroy resources\n");
    rc = 1;
  }
  if (config.dev_name)
    free((char *) config.dev_name);
  fprintf(stdout, "\ntest result is %d\n", rc);
  return rc;
}