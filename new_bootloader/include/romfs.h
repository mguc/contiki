#ifndef _ROMFS_H
#define _ROMFS_H

#ifdef __cplusplus
extern "C"{
#endif

#include "romfs_image.h"
#include <stdbool.h>

int romfs_mount();
void romfs_unmount();

long romfs_get_file_length(const char* path);
uint8_t* romfs_allocate_and_get_file_content(const char* path, bool add_slash_zero, long* allocated_length);

#ifdef __cplusplus
}
#endif

#define __cfunc__ (char*)__func__
#endif
