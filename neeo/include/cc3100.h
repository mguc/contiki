#ifndef CC3100_H
#define CC3100_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
  unsigned int chip_id;
  unsigned int fw_version[4];
  unsigned int phy_version[4];
  unsigned int nwp_version[4];
  unsigned int rom_version;
  unsigned int host_version[4];
} CC3100_Version;

/**
  * function to get the version of the CC3100.
  * @param version pointer to a CC3100_Version struct.
  * @return error code, 0 if no error has occured.
  */
int cc3100_get_version(CC3100_Version *version);

/**
  * function to check if firmware is up-to-date.
  * @return error code, 0 if firmware is up-to-date, 1 if firmware needs to be updated.
  */
int cc3100_check_version();
/**
  * function to update the firmware of the CC3100.
  * @return error code, 0 if firmware was updated correctly.
  */
int cc3100_update();

#ifdef __cplusplus
}
#endif

#endif
