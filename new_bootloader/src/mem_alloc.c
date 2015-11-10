/*
 * MamAlloc.c
 *
 *  Created on: 26 Mar 2015
 *      Author: ws
 */

#include <cyg/kernel/kapi.h>
#include "mem_alloc.h"

cyg_handle_t varpool;
cyg_mempool_var var;

void* custom_malloc(size_t size) {
//    return (void*)cyg_mempool_var_alloc(varpool, size);
    // ensure we allocate a multiple of 64b (including the chunk header)
    const int chunkHeaderSize = 16;
    const int roundedSize = 64;
    size += chunkHeaderSize;
    int kb = size / roundedSize;
    const int overflow = size % roundedSize;
    if(overflow) kb++;
    void* ptr = (void*)cyg_mempool_var_alloc(varpool, kb*roundedSize-chunkHeaderSize);

    return ptr;
}

void custom_free(void* p) {
    cyg_mempool_var_free(varpool, p);
}

void custom_mempool_init() {
#define POOL_SIZE (32*1024*1024)
    cyg_mempool_var_create((void*)(0xC0000000), POOL_SIZE, &varpool, &var);
}

int custom_freemem() {
    cyg_mempool_info info;
    cyg_mempool_var_get_info(varpool, &info);
    return info.freemem;
}

int custom_totalmem() {
    cyg_mempool_info info;
    cyg_mempool_var_get_info(varpool, &info);
    return info.size;
}
