#ifndef PROJECT_TR2_COM_FIRMWARE_H
#define PROJECT_TR2_COM_FIRMWARE_H

#define MAC_CONFIG_NULLRDC                    0
#define MAC_CONFIG_CONTIKIMAC                 1
/* Select a MAC configuration */
#define MAC_CONFIG MAC_CONFIG_NULLRDC

#undef NETSTACK_CONF_MAC
#undef NETSTACK_CONF_RDC
#undef NETSTACK_CONF_FRAMER

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
#define RF_CHANNEL 26

/* CoAP configuration */
#define REST_MAX_CHUNK_SIZE     80
#define COAP_MAX_HEADER_SIZE    176


#endif /* PROJECT_TR2_COM_FIRMWARE_H */