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
#define AES_HOUSEHOLD_KEY ((uint8_t*)"yoyoyoyoyoyoyoyo") /* store this in static mem */

/*---------------------------------------------------------------------------*/
#define PORT_INVITATIONS 200
#define PORT_ENCRYPTED 201

#define SEND_INVITATION_PERIOD (30 * CLOCK_SECOND)
#define SEND_ENCRYPTED_PERIOD (13 * CLOCK_SECOND)

static uip_ipaddr_t multicast_addr;
/*---------------------------------------------------------------------------*/
static struct udp_socket invitation;
static struct ctimer invitation_ctimer;
static struct ctimer return_ctimer;
/*---------------------------------------------------------------------------*/
static struct udp_socket encrypted;
static struct ctimer encrypted_ctimer;
/*---------------------------------------------------------------------------*/
PROCESS(aes_brain_process, "Test invitations brain process");
AUTOSTART_PROCESSES(&aes_brain_process);
/*---------------------------------------------------------------------------*/
static void
return_to_household_key(void* ptr)
{
  CCM_STAR.set_key(AES_HOUSEHOLD_KEY);
}
/*---------------------------------------------------------------------------*/
static void
send_invitation(void* ptr)
{
  CCM_STAR.set_key(AES_INVITE_KEY);
  char* buf = "I am brain, you are invited, come join me";
  printf("sending invitation: '%s'\n", buf);
  udp_socket_sendto(&invitation, buf, strlen(buf), &multicast_addr,
                    PORT_INVITATIONS);

  ctimer_set(&return_ctimer, 3 * CLOCK_SECOND, return_to_household_key, NULL);

  /* Invite again? */
  /*ctimer_set(&invitation_ctimer, SEND_INVITATION_PERIOD, send_invitation, NULL);*/
}
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
  printf("\n");
}
/*---------------------------------------------------------------------------*/
static void
send_encrypted(void* ptr)
{
  char* buf = "I am brain, and this is just some regular encrypted data";
  printf("sending encrypted: '%s'\n", buf);
  udp_socket_sendto(&encrypted, buf, strlen(buf), &multicast_addr,
                    PORT_ENCRYPTED);
  ctimer_set(&encrypted_ctimer, SEND_ENCRYPTED_PERIOD, send_encrypted, NULL);
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
PROCESS_THREAD(aes_brain_process, ev, data)
{
  PROCESS_BEGIN();

  /* This is the "brain".
   * In contrast to the Neeo brain, this application creates a RPL network.
   * Part of this functionality should probably be moved to the NBR, or even
   * to the main MCU (the nodejs process).
   *
   * This brain handles two different AES keys: one pre-defined key used to invite
   * new Remote-devices to the network, and one random per-brain specific key
   * which is used for the main communication.
   *
   * Both keys should be permanently stored and supplied by the main MCU.
   */

  rpl_dag_root_init_dag();
  printf("initiating rpl dag\n");

  /* Apply household key */
  CCM_STAR.set_key(AES_HOUSEHOLD_KEY);

  uip_create_linklocal_allnodes_mcast(&multicast_addr);

  /* We're going to be sending invitation messages periodically via UDP.
   * In the end, we want to send these on-demand only */
  udp_socket_register(&invitation, NULL, invitation_receiver);
  udp_socket_bind(&invitation, PORT_INVITATIONS);
  udp_socket_connect(&invitation, NULL, PORT_INVITATIONS);
  ctimer_set(&invitation_ctimer, 1 * CLOCK_SECOND, send_invitation, NULL); /* Start sending invitations */

  /* We're also sending out regular network data periodically.
   * The Remotes cannot receive these unless they have joined the network. */
  udp_socket_register(&encrypted, NULL, encrypted_receiver);
  udp_socket_bind(&encrypted, PORT_ENCRYPTED);
  udp_socket_connect(&encrypted, NULL, PORT_ENCRYPTED);
  ctimer_set(&encrypted_ctimer, 1 * CLOCK_SECOND, send_encrypted, NULL); /* Start sending encrypted data */

  printf("I'll send a single invitation in 30 seconds...\n");
  ctimer_set(&invitation_ctimer, 30 * CLOCK_SECOND, send_invitation, NULL);

  PROCESS_WAIT_UNTIL(0);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
