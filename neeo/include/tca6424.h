#ifndef TCA6424_H
#define TCA6424_H

#ifdef __cplusplus
extern "C" {
#endif

bool tca6424_init(void);
bool tca6424_read_input(uint16_t *state, uint16_t *stateP2);

#ifdef __cplusplus
};
#endif


#endif
