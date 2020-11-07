project(RDMA_TEST)

cmake_minimum_required(VERSION 3.1)

set (CMAKE_CXX_STANDARD 11)

include_directories(include)

add_executable(opt src/option/opt.c)

add_executable(rdma_cc_server src/rdma_cc/server.cc)
target_link_libraries(rdma_cc_server ibverbs pthread)

add_executable(rdma_cc_client src/rdma_cc/client.cc)
target_link_libraries(rdma_cc_client ibverbs pthread)