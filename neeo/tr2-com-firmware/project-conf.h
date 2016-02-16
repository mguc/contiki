#ifndef PROJECT_TR2_COM_FIRMWARE_H
#define PROJECT_TR2_COM_FIRMWARE_H

#define MAC_CONFIG_NULLRDC                    0
#define MAC_CONFIG_CONTIKIMAC                 1
/* Select a MAC configuration */
#define MAC_CONFIG MAC_CONFIG_NULLRDC

#undef NETSTACK_CONF_MAC
#undef NETSTACK_CONF_RDC
#undef NETSTACK_CONF_FRAMER

// The difference between nullrdc and contikimac is the way of accessing the medium (radio). Nullrdc keeps the radio turned on in RX mode
// all the time and listen all activity on the radio. Of course during transmission the radio is switched to TX mode.
// The contikimac keeps the radio turned off most of the time. CPU wakes-up the radio periodically and listen if there is any activity.
// During transmission, radio starts TX only if there is a silence in the air.
// This differences results in different power consumption (radio in RX mode consumes more power than in TX mode).

#if MAC_CONFIG == MAC_CONFIG_NULLRDC

#define NETSTACK_CONF_MAC     csma_driver
#define NETSTACK_CONF_RDC     nullrdc_driver
#define NETSTACK_CONF_FRAMER  framer_802154

#elif MAC_CONFIG == MAC_CONFIG_CONTIKIMAC

#define NETSTACK_CONF_MAC     csma_driver
#define NETSTACK_CONF_RDC     contikimac_driver
#define NETSTACK_CONF_FRAMER  contikimac_framer

#undef   MICROMAC_CONF_AUTOACK
#define  MICROMAC_CONF_AUTOACK 1
#undef MIRCOMAC_CONF_BUF_NUM
#define MIRCOMAC_CONF_BUF_NUM 16

#else

#error Unsupported MAC configuration

#endif /* MAC_CONFIG */

#undef UART_BAUD_RATE
#define UART_BAUD_RATE UART_RATE_115200
#undef UART1_BAUD_RATE
#define UART1_BAUD_RATE UART_RATE_115200

#undef IEEE802154_CONF_PANID
#define IEEE802154_CONF_PANID 0xabcd

#undef RF_CHANNEL
#define RF_CHANNEL 14

#undef   MICROMAC_CONF_CHANNEL
#define  MICROMAC_CONF_CHANNEL  RF_CHANNEL

#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE    1280
#undef UIP_CONF_ROUTER
#define UIP_CONF_ROUTER 0

/* CoAP configuration */
#define REST_MAX_CHUNK_SIZE     1024
#undef COAP_MAX_HEADER_SIZE
#define COAP_MAX_HEADER_SIZE 	200

#define CSMA_CONF_MAX_NEIGHBOR_QUEUES 8

#define RPL_DIS_INTERVAL_CONF 1

/* default COAP_RESPONSE_TIMEOUT is 3 */
#define COAP_RESPONSE_TIMEOUT       3

/* default COAP_MAX_RETRANSMIT is 4 */
#define COAP_MAX_RETRANSMIT         3

#endif /* PROJECT_TR2_COM_FIRMWARE_H */
