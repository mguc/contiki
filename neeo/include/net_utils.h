#ifndef PING_TEST_H
#define PING_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

int ping(const char* ip_address, unsigned int num_of_packets);
void get_mac_address(void);
void get_ip_config(void);

#ifdef __cplusplus
};
#endif

#endif /* PING_TEST_H */