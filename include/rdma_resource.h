//
// Created by wangxinshuo on 2020/11/7.
//

#ifndef RDMA_TEST_INCLUDE_RDMA_RESOURCE_H_
#define RDMA_TEST_INCLUDE_RDMA_RESOURCE_H_

#include "rdma_structure.h"
#include "rdma_socket.h"
#include "rdma_predefine.h"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

/******************************************************************************
* Function: resources_init
*
* Input
* res pointer to resources structure
*
* Output
* res is initialized
*
* Returns
* none
*
* Description
* res is initialized to default values
******************************************************************************/
static void resources_init(struct resources *res) {
  memset(res, 0, sizeof *res);
  res->sock = -1;
}

/******************************************************************************
* Function: resources_create
*
* Input
* res pointer to resources structure to be filled in
*
* Output
* res filled in with resources
*
* Returns
* 0 on success, 1 on failure
*
* Description
*
* This function creates and allocates all necessary system resources. These
* are stored in res.
*****************************************************************************/
static int resources_create(struct resources *res, struct config_t *config) {
  ibv_device **dev_list = nullptr;
  ibv_qp_init_attr qp_init_attr{};
  ibv_device *ib_dev = nullptr;
  size_t size;
  int i;
  int mr_flags = 0;
  int cq_size = 0;
  int num_devices;
  int rc = 0;
  /* if client side */
  if (config->server_name) {
    res->sock = sock_connect(config->server_name, config->tcp_port);
    if (res->sock < 0) {
      fprintf(stderr, "failed to establish TCP connection to server %s, port %d\n",
              config->server_name, config->tcp_port);
      rc = -1;
      goto resources_create_exit;
    }
  } else {
    fprintf(stdout, "waiting on port %d for TCP connection\n", config->tcp_port);
    res->sock = sock_connect(nullptr, config->tcp_port);
    if (res->sock < 0) {
      fprintf(stderr, "failed to establish TCP connection with client on port %d\n",
              config->tcp_port);
      rc = -1;
      goto resources_create_exit;
    }
  }
  fprintf(stdout, "TCP connection was established\n");
  fprintf(stdout, "searching for IB devices in host\n");
  /* get device names in the system */
  dev_list = ibv_get_device_list(&num_devices);
  if (!dev_list) {
    fprintf(stderr, "failed to get IB devices list\n");
    rc = 1;
    goto resources_create_exit;
  }
  /* if there isn't any IB device in host */
  if (!num_devices) {
    fprintf(stderr, "found %d device(s)\n", num_devices);
    rc = 1;
    goto resources_create_exit;
  }
  fprintf(stdout, "found %d device(s)\n", num_devices);
  /* search for the specific device we want to work with */
  for (i = 0; i < num_devices; i++) {
    if (!config->dev_name) {
      config->dev_name = strdup(ibv_get_device_name(dev_list[i]));
      fprintf(stdout, "device not specified, using first one found: %s\n",
              config->dev_name);
    }
    if (!strcmp(ibv_get_device_name(dev_list[i]), config->dev_name)) {
      ib_dev = dev_list[i];
      break;
    }
  }
  /* if the device wasn't found in host */
  if (!ib_dev) {
    fprintf(stderr, "IB device %s wasn't found\n", config->dev_name);
    rc = 1;
    goto resources_create_exit;
  }
  /* get device handle */
  res->ib_ctx = ibv_open_device(ib_dev);
  if (!res->ib_ctx) {
    fprintf(stderr, "failed to open device %s\n", config->dev_name);
    rc = 1;
    goto resources_create_exit;
  }
  /* We are now done with device list, free it */
  ibv_free_device_list(dev_list);
  dev_list = nullptr;
  ib_dev = nullptr;
  /* query port properties */
  if (ibv_query_port(res->ib_ctx, config->ib_port,
                     &res->port_attr)) {
    fprintf(stderr, "ibv_query_port on port %u failed\n", config->ib_port);
    rc = 1;
    goto resources_create_exit;
  }
  /* allocate Protection Domain */
  res->pd = ibv_alloc_pd(res->ib_ctx);
  if (!res->pd) {
    fprintf(stderr, "ibv_alloc_pd failed\n");
    rc = 1;
    goto resources_create_exit;
  }
  /* each side will send only one WR, so Completion Queue with 1 entry is enough */
  cq_size = 1;
  res->cq = ibv_create_cq(res->ib_ctx, cq_size, nullptr,
                          nullptr, 0);
  if (!res->cq) {
    fprintf(stderr, "failed to create CQ with %u entries\n", cq_size);
    rc = 1;
    goto resources_create_exit;
  }
  /* allocate the memory buffer that will hold the data */
  size = kMemSize;
  res->buf = (char *) malloc(size);
  if (!res->buf) {
    fprintf(stderr, "failed to malloc %zu bytes to memory buffer\n", size);
    rc = 1;
    goto resources_create_exit;
  }
  memset(res->buf, 0, size);
  /* only in the server side put the message in the memory buffer */
  if (!config->server_name) {
    strcpy(res->buf, msg);
    fprintf(stdout, "going to send the message: '%s'\n", res->buf);
  } else
    memset(res->buf, 0, size);
  /* register the memory buffer */
  mr_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
  res->mr = ibv_reg_mr(res->pd, res->buf, size, mr_flags);
  if (!res->mr) {
    fprintf(stderr, "ibv_reg_mr failed with mr_flags=0x%x\n", mr_flags);
    rc = 1;
    goto resources_create_exit;
  }
  fprintf(stdout, "MR was registered with addr=%p, lkey=0x%x, rkey=0x%x, flags=0x%x\n",
          res->buf, res->mr->lkey, res->mr->rkey, mr_flags);
  /* create the Queue Pair */
  memset(&qp_init_attr, 0, sizeof(qp_init_attr));
  qp_init_attr.qp_type = IBV_QPT_RC;
  qp_init_attr.sq_sig_all = 1;
  qp_init_attr.send_cq = res->cq;
  qp_init_attr.recv_cq = res->cq;
  qp_init_attr.cap.max_send_wr = 1;
  qp_init_attr.cap.max_recv_wr = 1;
  qp_init_attr.cap.max_send_sge = 1;
  qp_init_attr.cap.max_recv_sge = 1;
  res->qp = ibv_create_qp(res->pd, &qp_init_attr);
  if (!res->qp) {
    fprintf(stderr, "failed to create QP\n");
    rc = 1;
    goto resources_create_exit;
  }
  fprintf(stdout, "QP was created, QP number=0x%x\n", res->qp->qp_num);
  resources_create_exit:
  if (rc) {
    /* Error encountered, cleanup */
    if (res->qp) {
      ibv_destroy_qp(res->qp);
      res->qp = nullptr;
    }
    if (res->mr) {
      ibv_dereg_mr(res->mr);
      res->mr = nullptr;
    }
    if (res->buf) {
      free(res->buf);
      res->buf = nullptr;
    }
    if (res->cq) {
      ibv_destroy_cq(res->cq);
      res->cq = nullptr;
    }
    if (res->pd) {
      ibv_dealloc_pd(res->pd);
      res->pd = nullptr;
    }
    if (res->ib_ctx) {
      ibv_close_device(res->ib_ctx);
      res->ib_ctx = nullptr;
    }
    if (dev_list) {
      ibv_free_device_list(dev_list);
      dev_list = nullptr;
    }
    if (res->sock >= 0) {
      if (close(res->sock))
        fprintf(stderr, "failed to close socket\n");
      res->sock = -1;
    }
  }
  return rc;
}

/******************************************************************************
* Function: resources_destroy
*
* Input
* res pointer to resources structure
*
* Output
* none
*
* Returns
* 0 on success, 1 on failure
*
* Description
* Cleanup and deallocate all resources used
******************************************************************************/
static int resources_destroy(struct resources *res) {
  int rc = 0;
  if (res->qp)
    if (ibv_destroy_qp(res->qp)) {
      fprintf(stderr, "failed to destroy QP\n");
      rc = 1;
    }
  if (res->mr)
    if (ibv_dereg_mr(res->mr)) {
      fprintf(stderr, "failed to deregister MR\n");
      rc = 1;
    }
  if (res->buf)
    free(res->buf);
  if (res->cq)
    if (ibv_destroy_cq(res->cq)) {
      fprintf(stderr, "failed to destroy CQ\n");
      rc = 1;
    }
  if (res->pd)
    if (ibv_dealloc_pd(res->pd)) {
      fprintf(stderr, "failed to deallocate PD\n");
      rc = 1;
    }
  if (res->ib_ctx)
    if (ibv_close_device(res->ib_ctx)) {
      fprintf(stderr, "failed to close device context\n");
      rc = 1;
    }
  if (res->sock >= 0)
    if (close(res->sock)) {
      fprintf(stderr, "failed to close socket\n");
      rc = 1;
    }
  return rc;
}

#endif //RDMA_TEST_INCLUDE_RDMA_RESOURCE_H_
