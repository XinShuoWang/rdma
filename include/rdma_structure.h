//
// Created by wangxinshuo on 2020/11/7.
//

#ifndef RDMA_TEST_INCLUDE_RDMA_STRUCTURE_H_
#define RDMA_TEST_INCLUDE_RDMA_STRUCTURE_H_

#include <cstdint>
#include <sys/types.h>
#include <infiniband/verbs.h>

/* structure of test parameters */
struct config_t {
  const char *dev_name; /* IB device name */
  char *server_name;      /* server host name */
  u_int32_t tcp_port;      /* server TCP port */
  int ib_port;          /* local IB port to work with */
  int gid_idx;          /* gid index to use */
};

/* structure to exchange data which is needed to connect the QPs */
struct cm_con_data_t {
  uint64_t addr;     /* Buffer address */
  uint32_t rkey;     /* Remote key */
  uint32_t qp_num; /* QP number */
  uint16_t lid;     /* LID of the IB port */
  uint8_t gid[16]; /* gid */
} __attribute__((packed));

/* structure of system resources */
struct resources {
  struct ibv_device_attr device_attr;
  /* Device attributes */
  struct ibv_port_attr port_attr;       /* IB port attributes */
  struct cm_con_data_t remote_props; /* values to connect to remote side */
  struct ibv_context *ib_ctx;           /* device handle */
  struct ibv_pd *pd;                   /* PD handle */
  struct ibv_cq *cq;                   /* CQ handle */
  struct ibv_qp *qp;                   /* QP handle */
  struct ibv_mr *mr;                   /* MR handle for buf */
  /* memory buffer pointer, used for RDMA and send ops */
  char *buf;
  int sock;                           /* TCP socket file descriptor */
};

#endif //RDMA_TEST_INCLUDE_RDMA_STRUCTURE_H_
