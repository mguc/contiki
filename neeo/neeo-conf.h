#ifndef PROJECT_NEEO_H
#define PROJECT_NEEO_H

#define ENERGEST_CONF_ON 1
#define RIMESTATS_CONF_ENABLED 1
#define RIMESTATS_CONF_ON 1

/* Configure the radio to *not* generate or handle acks.
 * Instead our RDC should both transmit and receive acks. */
#define NULLRDC_CONF_802154_AUTOACK  1 /* handle incoming acks */
#define NULLRDC_CONF_ACK_WAIT_TIME (4UL * RTIMER_SECOND / 1000UL)
#undef MIRCOMAC_CONF_BUF_NUM
#define MIRCOMAC_CONF_BUF_NUM 4

/* AES-CCM via NonCoreSec Contiki core MODULE
 * XXX Note that the network key *must* be set in the application */
#if 0
#define LLSEC802154_CONF_SECURITY_LEVEL 6
#define NETSTACK_CONF_LLSEC noncoresec_driver
#undef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER noncoresec_framer
#endif

#undef SLIP_BRIDGE_CONF_NO_PUTCHAR
#define SLIP_BRIDGE_CONF_NO_PUTCHAR 0

#define IEEE802154_CONF_PANID           0xABCD

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

#else

#error Unsupported MAC configuration

#endif /* MAC_CONFIG */

#undef UART_BAUD_RATE
#define UART_BAUD_RATE UART_RATE_115200
#undef UART1_BAUD_RATE
#define UART1_BAUD_RATE UART_RATE_115200

#undef IEEE802154_CONF_PANID
#define IEEE802154_CONF_PANID 0xabcd

#undef   MICROMAC_CONF_CHANNEL
#define  MICROMAC_CONF_CHANNEL  14

#if CONTIKI_TARGET_EXP5438
/* Let's keep the default settings, to preserve some memory for simulations */
#else /* CONTIKI_TARGET_EXP5438 */

#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE    1280

/* CoAP configuration */
#undef REST_MAX_CHUNK_SIZE
#define REST_MAX_CHUNK_SIZE     1024
#undef COAP_MAX_HEADER_SIZE
#define COAP_MAX_HEADER_SIZE 	  200
#undef COAP_MAX_RETRANSMIT
#define COAP_MAX_RETRANSMIT     1

/* CSMA configuration */
#undef SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS
#define SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS 1
#undef CSMA_CONF_MAX_MAC_TRANSMISSIONS
#define CSMA_CONF_MAX_MAC_TRANSMISSIONS 1

#endif /* CONTIKI_TARGET_EXP5438 */

#endif /* PROJECT_NEEO_H */
