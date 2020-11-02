project(RDMA_TEST)

cmake_minimum_required(VERSION 2.8)

add_executable(rdma_server src/rdma/server.c)
target_link_libraries(rdma_server ibverbs)

add_executable(rdma_client src/rdma/client.c)
target_link_libraries(rdma_client ibverbs)

add_executable(opt src/option/opt.c)