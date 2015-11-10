/*
 * MamAlloc.h
 *
 *  Created on: 26 Mar 2015
 *      Author: ws
 */

#ifndef NEEO_INC_MEM_ALLOC_H_
#define NEEO_INC_MEM_ALLOC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <cyg/memalloc/kapi.h>

void custom_mempool_init();
void* custom_malloc(size_t size);
void custom_free(void* p);
int custom_freemem();
int custom_totalmem();

#ifdef __cplusplus
};
#endif


#endif /* NEEO_INC_MEM_ALLOC_H_ */
