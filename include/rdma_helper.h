//
// Created by wangxinshuo on 2020/11/7.
//

#ifndef RDMA_TEST_INCLUDE_RDMA_HELPER_H_
#define RDMA_TEST_INCLUDE_RDMA_HELPER_H_

#include "rdma_structure.h"

#include <stdio.h>

/******************************************************************************
* Function: print_config
*
* Input
* none
*
* Output
* none
*
* Returns
* none
*
* Description
* Print out config information
******************************************************************************/
static void print_config(struct config_t *config) {
  fprintf(stdout, " ------------------------------------------------\n");
  fprintf(stdout, " Device name : \"%s\"\n", config->dev_name);
  fprintf(stdout, " IB port : %u\n", config->ib_port);
  if (config->server_name)
    fprintf(stdout, " IP : %s\n", config->server_name);
  fprintf(stdout, " TCP port : %u\n", config->tcp_port);
  if (config->gid_idx >= 0)
    fprintf(stdout, " GID index : %u\n", config->gid_idx);
  fprintf(stdout, " ------------------------------------------------\n\n");
}

/******************************************************************************
* Function: usage
*
* Input
* argv0 command line arguments
*
* Output
* none
*
* Returns
* none
*
* Description
* print a description of command line syntax
******************************************************************************/
static void usage(const char *argv0) {
  fprintf(stdout, "Usage:\n");
  fprintf(stdout, " %s start a server and wait for connection\n", argv0);
  fprintf(stdout, " %s <host> connect to server at <host>\n", argv0);
  fprintf(stdout, "\n");
  fprintf(stdout, "Options:\n");
  fprintf(stdout, " -p, --port <port> listen on/connect to port <port> (default 18515)\n");
  fprintf(stdout, " -d, --ib-dev <dev> use IB device <dev> (default first device found)\n");
  fprintf(stdout, " -i, --ib-port <port> use port <port> of IB device (default 1)\n");
  fprintf(stdout, " -g, --gid_idx <git index> gid index to be used in GRH (default not used)\n");
}

#endif //RDMA_TEST_INCLUDE_RDMA_HELPER_H_
