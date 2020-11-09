//
// Created by wangxinshuo on 2020/11/7.
//

#include "rdma_structure.h"
#include "rdma_resource.h"
#include "rdma_poll.h"
#include "rdma_connect.h"

#include <chrono>
#include <thread>

class RdmaServer {
 private:
  resources resources_{};
  config_t config_ = {
      nullptr,  /* dev_name */
      nullptr,  /* server_name */
      19875, /* tcp_port */
      1,       /* ib_port */
      -1 /* gid_idx */
  };

  bool have_killed;
 public:
  RdmaServer() {
    have_killed = false;
    char temp_char;
    resources_init(&resources_);
    resources_create(&resources_, &config_);
    connect_qp(&resources_, &config_);
    post_send(&resources_, IBV_WR_SEND);
    poll_completion(&resources_);
//    sock_sync_data(resources_.sock, 1, "R", &temp_char);
//    sock_sync_data(resources_.sock, 1, "W", &temp_char);
  }

  [[noreturn]] void spin() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::minutes(1));
    }
  }

  void kill(){
    resources_destroy(&resources_);
    if (config_.dev_name) {
      free((char *) config_.dev_name);
    }
    have_killed = true;
  }

  ~RdmaServer() {
    if(!have_killed){
      resources_destroy(&resources_);
      if (config_.dev_name) {
        free((char *) config_.dev_name);
      }
    }
  }
};

int main(){
  RdmaServer server;
  server.spin();
  return 0;
}