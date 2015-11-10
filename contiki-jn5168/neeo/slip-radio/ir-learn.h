#ifndef IR_LEARN_H
#define IR_LEARN_H

void ir_learn_init(void);
uint8_t ir_learn_start(uint32_t sample_num);
void ir_learn_stop(void);
void ir_learn_send_results(void);

PROCESS_NAME(ir_learn_process);

#endif /* IR_LEARN_H */