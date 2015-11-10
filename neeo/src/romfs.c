#include <stdint.h>
#include "romfs.h"
#include "log_helper.h"
#include <stdio.h>
#include <sys/stat.h>
#include <cyg/fileio/fileio.h>

static int is_mounted = 0;
static FILE* open_file(const char* path);

extern void* custom_malloc(size_t size);
extern void custom_free(void* p);

int romfs_mount()
{
	char addr[20];
	sprintf(addr, "0x%08X", (unsigned int)romfs);
    is_mounted = (mount(addr, "/rom", "romfs") == 0);
    return is_mounted ? 0 : -1;
}

void romfs_unmount()
{
    if (!is_mounted)
    {
        return;
    }

    is_mounted = 0;
    umount("/rom");
}

long romfs_get_file_length(const char* path)
{
    char buffer[128];
    buffer[0] = 0;
    strcat(buffer, "/rom/"); 
    strncat(buffer, path, 120);

    struct stat file_stat;
    if (stat(buffer, &file_stat) == -1)
    {
        return -1;
    }

    return file_stat.st_size;
}

long romfs_get_file_content(const char* path, uint8_t* buffer, long max_length)
{
    FILE *f = open_file(path);
    if (f == NULL)
    {
        return -1;
    }

    uint8_t* position = buffer;
    long counter = 0;
    char ch;

    while ((fread(&ch, 1, 1, f) == 1) && (counter < max_length)) 
    {
        *position = ch;
        position++;
        counter++;
    }
    fclose(f);

    return counter;
}

uint8_t* romfs_allocate_and_get_file_content(const char* path, bool add_slash_zero, long* allocated_length)
{
    long length = romfs_get_file_length(path);
    if (length == -1)
    {
        log_msg(LOG_ERROR, __cfunc__, "%s file not found", path);
        return NULL;
    }
    else
    {
        *allocated_length = (add_slash_zero ? length + 1 : length);
        uint8_t* buffer = (uint8_t*) custom_malloc(*allocated_length);
        if (romfs_get_file_content(path, buffer, length) != length)
        {
            log_msg(LOG_ERROR, __cfunc__, "error while reading data from file %s", path);
            custom_free(buffer);
            return NULL;
        }
        if (add_slash_zero)
        {
            buffer[length] = 0;
        }

        return buffer;
    }
}

static FILE* open_file(const char* path)
{
    char buffer[128];
    buffer[0] = 0;
    strcat(buffer, "/rom/"); 
    strncat(buffer, path, 120);

    return fopen(buffer, "rb");
}


