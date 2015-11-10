#ifndef JOY_H
#define JOY_H

#ifdef __cplusplus
extern "C" {
#endif

#define JOY_LEFT    0x08
#define JOY_RIGHT   0x01
#define JOY_UP      0x04
#define JOY_DOWN    0x02
#define JOY_SELECT  0x10


void joy_init(void);
cyg_uint8 joy_query(void);

#ifdef __cplusplus
};
#endif


#endif
