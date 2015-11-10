#ifndef GT9147_H
#define GT9147_H

#include <cyg/kernel/kapi.h>

#ifdef __cplusplus
extern "C" {
#endif

struct GTP_DATA {
    cyg_uint8 touch_flag;
    cyg_uint16 x;
    cyg_uint16 y;
};

void gtp_get_points(struct GTP_DATA* touch);
bool gtp_read_id(char* buffer);

#ifdef __cplusplus
}
#endif

#endif /* GT9147_H */
