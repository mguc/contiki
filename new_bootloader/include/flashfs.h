#ifndef _FLASHFS_H
#define _FLASHFS_H

#ifdef __cplusplus
extern "C"{
#endif

#include <stdbool.h>

uint32_t flashfs_probe(void);
int flashfs_mount();
void flashfs_erase(void);

#ifdef __cplusplus
}
#endif

#define __cfunc__ (char*)__func__
#endif /* _FLASHFS_H */
