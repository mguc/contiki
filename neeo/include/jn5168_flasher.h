#ifndef JN5168_FLASHER_H
#define JN5168_FLASHER_H

#ifdef __cplusplus
extern "C" {
#endif

void jn_init_gpio(void);
int jn_init_bootloader(void);
int jn_read_mac(cyg_uint8* mac);
int jn_read_license(cyg_uint8* license);
int jn_upload(cyg_uint8* Program, cyg_uint32 Length);
void jn_power_on(bool state);
void jn_reset(void);

#ifdef __cplusplus
};
#endif

#endif