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
#include <infiniband/verbs.h>

struct config_t config = {
    NULL,  /* dev_name */
    NULL,  /* server_name */
    19875, /* tcp_port */
    1,       /* ib_port */
    -1 /* gid_idx */
};

int main() {
  struct resources res;
  int rc = 1;
  char temp_char;

  config.tcp_port = 19875;

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
  /* let the server post the sr */
  if (post_send(&res, IBV_WR_SEND)) {
    fprintf(stderr, "failed to post sr\n");
    goto main_exit;
  }
  /* in both sides we expect to get a completion */
  if (poll_completion(&res)) {
    fprintf(stderr, "poll completion failed\n");
    goto main_exit;
  }

  /* after polling the completion we have the message in the client buffer too */
  strcpy(res.buf, RDMAMSGR);

  /* 进行同步，因此我们确保服务器端在客户端尝试读取数据之前已准备好数据 */
  /* 只是来回发送一个虚拟字符 */
  if (sock_sync_data(res.sock, 1, "R", &temp_char)) {
    fprintf(stderr, "sync error before RDMA ops\n");
    rc = 1;
    goto main_exit;
  }

  /* 同步后，服务器将知道客户端已完成其内存的处理 */
  /* 只是来回发送一个虚拟字符 */
  if (sock_sync_data(res.sock, 1, "W", &temp_char)) {
    fprintf(stderr, "sync error after RDMA ops\n");
    rc = 1;
    goto main_exit;
  }

  fprintf(stdout, "Contents of server buffer: '%s'\n", res.buf);

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
