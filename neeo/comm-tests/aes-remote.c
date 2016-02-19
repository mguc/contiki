#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/leds.h"
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-dag-root.h"
#include "net/ip/udp-socket.h"
#include "net/ip/uip-debug.h"

#include "net/llsec/llsec802154.h"
#include "lib/ccm-star.h"
#include "net/llsec/ccm-star-packetbuf.h"
#include "net/mac/frame802154.h"

#include <stdio.h>
#include <string.h>

#define AES_INVITE_KEY ((uint8_t*)"abbaabbaabbaabba") /* store this in static mem */
/*#define AES_HOUSEHOLD_KEY "we don't know this one yet" */

/*---------------------------------------------------------------------------*/
#define PORT_INVITATIONS 200
#define PORT_ENCRYPTED 201

#define SEND_ENCRYPTED_PERIOD (27 * CLOCK_SECOND)

static uip_ipaddr_t multicast_addr;
/*---------------------------------------------------------------------------*/
static struct udp_socket invitation;
/*---------------------------------------------------------------------------*/
static struct udp_socket encrypted;
static struct ctimer encrypted_ctimer;
/*---------------------------------------------------------------------------*/
PROCESS(aes_remote_process, "Test invitations remote process");
AUTOSTART_PROCESSES(&aes_remote_process);
/*---------------------------------------------------------------------------*/
static void
invitation_receiver(struct udp_socket *c,
             void *ptr,
             const uip_ipaddr_t *sender_addr,
             uint16_t sender_port,
             const uip_ipaddr_t *receiver_addr,
             uint16_t receiver_port,
             const uint8_t *data,
             uint16_t datalen)
{
  printf("got invitation message from: ");
  uip_debug_ipaddr_print(sender_addr);
  printf(": '%s'\n", data);

  printf("TODO XXX JOIN THIS NETWORK?\n");
}
/*---------------------------------------------------------------------------*/
static void
send_encrypted(void* ptr)
{
  ctimer_set(&encrypted_ctimer, SEND_ENCRYPTED_PERIOD, send_encrypted, NULL);

  if(rpl_get_any_dag() == NULL) {
    /* We only send data if we've joined RPL */
    printf("can't send encrypted, no RPL network\n");
    return;
  }

  char* buf = "I am remote, and this is just some regular encrypted data";
  printf("sending encrypted: '%s'\n", buf);
  udp_socket_sendto(&encrypted, buf, strlen(buf), &multicast_addr,
                    PORT_ENCRYPTED);
}
/*---------------------------------------------------------------------------*/
static void
encrypted_receiver(struct udp_socket *c,
             void *ptr,
             const uip_ipaddr_t *sender_addr,
             uint16_t sender_port,
             const uip_ipaddr_t *receiver_addr,
             uint16_t receiver_port,
             const uint8_t *data,
             uint16_t datalen)
{
  printf("got encrypted message from: ");
  uip_debug_ipaddr_print(sender_addr);
  printf(": '%s'\n", data);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(aes_remote_process, ev, data)
{
  PROCESS_BEGIN();

  /* This is the "remote".
   * This device is just an ordinary RPL-capable device, that joins any network
   * it can see. The problem is that, at startup, it cannot see any network.
   *
   * Via Brain-sent invites it can read out a network key, and when applied, it
   * can finally join a RPL network.
   *
   * Both keys should be permanently stored and supplied by the main MCU.
   */

  /* Apply invite key */
  CCM_STAR.set_key(AES_INVITE_KEY);

  uip_create_linklocal_allnodes_mcast(&multicast_addr);

  /* We're going to be listening for invitation messages via UDP */
  udp_socket_register(&invitation, NULL, invitation_receiver);
  udp_socket_bind(&invitation, PORT_INVITATIONS);
  udp_socket_connect(&invitation, NULL, PORT_INVITATIONS);

  /* We're also sending out regular network data periodically.
   * The Brain (or any other device) cannot receive these unless we're using
   * the proper network key */
  udp_socket_register(&encrypted, NULL, encrypted_receiver);
  udp_socket_bind(&encrypted, PORT_ENCRYPTED);
  udp_socket_connect(&encrypted, NULL, PORT_ENCRYPTED);
  ctimer_set(&encrypted_ctimer, 1 * CLOCK_SECOND, send_encrypted, NULL); /* Start sending encrypted data */

  PROCESS_WAIT_UNTIL(0);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
