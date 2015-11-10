#ifndef FILE_DOWNLOAD_H
#define FILE_DOWNLOAD_H

#ifdef __cplusplus
extern "C" {
#endif

#define FD_SSID_NAME       "hrafn"            /* Access point name to connect to. */
#define FD_SEC_TYPE        SL_SEC_TYPE_WPA_WPA2 /* Security type of the Access piont */
#define FD_PASSKEY         "sagaohrafnkelu"     /* Password in case of secure AP */
#define FD_PASSKEY_LEN     pal_Strlen(PASSKEY)  /* Password length in case of secure AP */
#define HOST_NAME          "192.168.1.80"       /* HTTP server which provides data for neeo (can be alfanumeric address) */
#define HOST_PORT 3100


extern void* custom_malloc(size_t size);
extern void custom_free(void* p);

cyg_uint32 wifi_init(uint8_t * ssid, int ssid_length, uint8_t * pass, int pass_length);
int file_init_once(void);
cyg_uint32 wifi_deinit(void);
int wifi_download(const char* name, const char* host_ip, unsigned int* size, unsigned char** content);

typedef struct FileInfo_s{
    uint8_t* FilePtr;
    uint32_t Size;
}FileInfo_t;

#ifdef __cplusplus
};
#endif

#endif
