#ifndef IR_CONTROL_H
#define IR_CONTROL_H

int ir_start(const uint16_t *data, uint32_t len);
int ir_stop(void);
void ir_init(void);

PROCESS_NAME(ir_control_process);

#endif /* IR_CONTROL_H */