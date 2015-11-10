#ifndef TCA9535_H
#define TCA9535_H

#ifdef __cplusplus
extern "C" {
#endif

bool tca9535_init(void);
bool tca9535_read_input(uint16_t *state);

#ifdef __cplusplus
};
#endif


#endif
