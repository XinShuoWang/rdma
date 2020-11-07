//
// Created by wangxinshuo on 2020/11/7.
//

#ifndef RDMA_TEST_INCLUDE_RDMA_PREDEFINE_H_
#define RDMA_TEST_INCLUDE_RDMA_PREDEFINE_H_

#include <cstdint>
#include <byteswap.h>

const char msg[] = "";
const uint64_t kMaxPollCqTimeout = 2000;
const uint64_t kMemSize = 1 * 1024 * 1024 * 1024;

#if __BYTE_ORDER == __LITTLE_ENDIAN
static inline uint64_t htonll(uint64_t x) { return bswap_64(x); }
static inline uint64_t ntohll(uint64_t x) { return bswap_64(x); }
#elif __BYTE_ORDER == __BIG_ENDIAN
static inline uint64_t htonll(uint64_t x) { return x; }
static inline uint64_t ntohll(uint64_t x) { return x; }
#else
#error __BYTE_ORDER is neither __LITTLE_ENDIAN nor __BIG_ENDIAN
#endif

#endif //RDMA_TEST_INCLUDE_PREDEFINE_H_
