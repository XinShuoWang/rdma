//
// Created by wangxinshuo on 2020/11/7.
//

#ifndef RDMA_TEST_INCLUDE_RDMA_CONNECT_H_
#define RDMA_TEST_INCLUDE_RDMA_CONNECT_H_

#include "rdma_structure.h"
#include "rdma_predefine.h"
#include "rdma_socket.h"
#include "rdma_poll.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <cstdint>
#include <cinttypes>

#include <sys/time.h>
#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <sys/socket.h>
#include <netdb.h>

/******************************************************************************
* Function: modify_qp_to_init
*
* Input
* qp QP to transition
*
* Output
* none
*
* Returns
* 0 on success, ibv_modify_qp failure code on failure
*
* Description
* Transition a QP from the RESET to INIT state
******************************************************************************/
static int modify_qp_to_init(struct ibv_qp *qp, struct config_t *config) {
  ibv_qp_attr attr{};
  int flags;
  int rc;

  memset(&attr, 0, sizeof(attr));
  attr.qp_state = IBV_QPS_INIT;
  attr.port_num = config->ib_port;
  attr.pkey_index = 0;
  attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
  flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;

  rc = ibv_modify_qp(qp, &attr, flags);
  if (rc)
    fprintf(stderr, "failed to modify QP state to INIT\n");
  return rc;
}

/******************************************************************************
* Function: modify_qp_to_rtr
*
* Input
* qp QP to transition
* remote_qpn remote QP number
* dlid destination LID
* dgid destination GID (mandatory for RoCEE)
*
* Output
* none
*
* Returns
* 0 on success, ibv_modify_qp failure code on failure
*
* Description
* Transition a QP from the INIT to RTR state, using the specified QP number
******************************************************************************/
static int modify_qp_to_rtr(struct ibv_qp *qp, uint32_t remote_qpn,
                            uint16_t dlid, uint8_t *dgid, struct config_t *config) {
  ibv_qp_attr attr{};
  int flags;
  int rc;
  memset(&attr, 0, sizeof(attr));
  attr.qp_state = IBV_QPS_RTR;
  attr.path_mtu = IBV_MTU_256;
  attr.dest_qp_num = remote_qpn;
  attr.rq_psn = 0;
  attr.max_dest_rd_atomic = 1;
  attr.min_rnr_timer = 0x12;
  attr.ah_attr.is_global = 0;
  attr.ah_attr.dlid = dlid;
  attr.ah_attr.sl = 0;
  attr.ah_attr.src_path_bits = 0;
  attr.ah_attr.port_num = config->ib_port;
  if (config->gid_idx >= 0) {
    attr.ah_attr.is_global = 1;
    attr.ah_attr.port_num = 1;
    memcpy(&attr.ah_attr.grh.dgid, dgid, 16);
    attr.ah_attr.grh.flow_label = 0;
    attr.ah_attr.grh.hop_limit = 1;
    attr.ah_attr.grh.sgid_index = config->gid_idx;
    attr.ah_attr.grh.traffic_class = 0;
  }
  flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
      IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
  rc = ibv_modify_qp(qp, &attr, flags);
  if (rc)
    fprintf(stderr, "failed to modify QP state to RTR\n");
  return rc;
}

/******************************************************************************
* Function: modify_qp_to_rts
*
* Input
* qp QP to transition
*
* Output
* none
*
* Returns
* 0 on success, ibv_modify_qp failure code on failure
*
* Description
* Transition a QP from the RTR to RTS state
******************************************************************************/
static int modify_qp_to_rts(struct ibv_qp *qp) {
  ibv_qp_attr attr{};
  int flags;
  int rc;
  memset(&attr, 0, sizeof(attr));
  attr.qp_state = IBV_QPS_RTS;
  attr.timeout = 0x12;
  attr.retry_cnt = 6;
  attr.rnr_retry = 0;
  attr.sq_psn = 0;
  attr.max_rd_atomic = 1;
  flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
      IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
  rc = ibv_modify_qp(qp, &attr, flags);
  if (rc)
    fprintf(stderr, "failed to modify QP state to RTS\n");
  return rc;
}

/******************************************************************************
* Function: connect_qp
*
* Input
* res pointer to resources structure
*
* Output
* none
*
* Returns
* 0 on success, error code on failure
*
* Description
* Connect the QP. Transition the server side to RTR, sender side to RTS
******************************************************************************/
static int connect_qp(struct resources *res, struct config_t *config) {
  cm_con_data_t local_con_data{};
  cm_con_data_t remote_con_data{};
  cm_con_data_t tmp_con_data{};
  int rc = 0;
  char temp_char;
  ibv_gid my_gid{};

  // TODO: GET GID
  if (config->gid_idx >= 0) {
    rc = ibv_query_gid(res->ib_ctx, config->ib_port, config->gid_idx, &my_gid);
    if (rc) {
      fprintf(stderr, "could not get gid for port %d, index %d\n",
              config->ib_port, config->gid_idx);
      return rc;
    }
  } else
    memset(&my_gid, 0, sizeof my_gid);


  /* exchange using TCP sockets info required to connect QPs */
  local_con_data.addr = htonll((uintptr_t) res->buf);
  local_con_data.rkey = htonl(res->mr->rkey);
  local_con_data.qp_num = htonl(res->qp->qp_num);
  local_con_data.lid = htons(res->port_attr.lid);
  memcpy(local_con_data.gid, &my_gid, 16);
  fprintf(stdout, "\nLocal LID = 0x%x\n", res->port_attr.lid);

  // TODO: SYNC DATA
  if (sock_sync_data(res->sock, sizeof(struct cm_con_data_t),
                     (char *) &local_con_data, (char *) &tmp_con_data) < 0) {
    fprintf(stderr, "failed to exchange connection data between sides\n");
    rc = 1;
    goto connect_qp_exit;
  }

  // convert return data-structure into resources_
  remote_con_data.addr = ntohll(tmp_con_data.addr);
  remote_con_data.rkey = ntohl(tmp_con_data.rkey);
  remote_con_data.qp_num = ntohl(tmp_con_data.qp_num);
  remote_con_data.lid = ntohs(tmp_con_data.lid);
  memcpy(remote_con_data.gid, tmp_con_data.gid, 16);

  // TODO:SET REMOTE PROPS
  /* save the remote side attributes, we will need it for the post SR */
  res->remote_props = remote_con_data;
  fprintf(stdout, "Remote address = 0x%"
                  PRIx64
                  "\n", remote_con_data.addr);
  fprintf(stdout, "Remote rkey = 0x%x\n", remote_con_data.rkey);
  fprintf(stdout, "Remote QP number = 0x%x\n", remote_con_data.qp_num);
  fprintf(stdout, "Remote LID = 0x%x\n", remote_con_data.lid);
  if (config->gid_idx >= 0) {
    uint8_t *p = remote_con_data.gid;
    fprintf(stdout,
            "Remote GID =%02x:%02x:%02x:%02x:%02x:%02x"
            ":%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n ",
            p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8],
            p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
  }

  // TODO:QP IS INIT
  /* modify the QP to init */
  rc = modify_qp_to_init(res->qp, config);
  if (rc) {
    fprintf(stderr, "change QP state to INIT failed\n");
    goto connect_qp_exit;
  }
  /* let the client post RR to be prepared for incoming messages */
  if (config->server_name) {
    rc = post_receive(res);
    if (rc) {
      fprintf(stderr, "failed to post RR\n");
      goto connect_qp_exit;
    }
  }

  // TODO:QP IS RTR
  /* modify the QP to RTR */
  rc = modify_qp_to_rtr(res->qp, remote_con_data.qp_num,
                        remote_con_data.lid, remote_con_data.gid, config);
  if (rc) {
    fprintf(stderr, "failed to modify QP state to RTR\n");
    goto connect_qp_exit;
  }

  // TODO:QP READY TO RTS
  rc = modify_qp_to_rts(res->qp);
  if (rc) {
    fprintf(stderr, "failed to modify QP state to RTR\n");
    goto connect_qp_exit;
  }
  fprintf(stdout, "QP state was change to RTS\n");
  /* sync to make sure that both sides are in states that
   * they can connect to prevent packet loose */
  /* just send a dummy char back and forth */
  if (sock_sync_data(res->sock, 1, "Q", &temp_char)) {
    fprintf(stderr, "sync error after QPs are were moved to RTS\n");
    rc = 1;
  }

  connect_qp_exit:
  return rc;
}

#endif //RDMA_TEST_INCLUDE_RDMA_CONNECT_H_
