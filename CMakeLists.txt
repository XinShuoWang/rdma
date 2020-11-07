project(RDMA_TEST)

cmake_minimum_required(VERSION 2.8)

include_directories(include)

add_executable(opt src/option/opt.c)

add_executable(rdma_c_server src/rdma_c/server.c)
target_link_libraries(rdma_c_server ibverbs)

add_executable(rdma_c_client src/rdma_c/client.c)
target_link_libraries(rdma_c_client ibverbs)