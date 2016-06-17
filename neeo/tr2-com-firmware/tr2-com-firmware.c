#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include <string.h>
#include "dev/watchdog.h"
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#include "sys/etimer.h"
#include "lib/ringbuf.h"
#include "version.h"
#include "serial-protocol.h"
#include "tr2-common.h"
#include "rtc.h"
#include "er-coap-engine.h"
#include "rest_server.h"

#define DEBUG_LEVEL DEBUG_NONE
#include "log_helper.h"
/*---------------------------------------------------------------------------*/
#define BRAIN_PORT        3100
#define BRAIN_COAP_PORT   3901
#define FW_MAJOR_VERSION    "25"

#define FIRST_CHANNEL 11
#define LAST_CHANNEL 26
#define CP6_LIST_MAX 4
#define DISCOVERY_DUTY_CYCLE CLOCK_SECOND/10
#define OPERATING_CHANNELS 15
const radio_value_t operating_channels[OPERATING_CHANNELS] = \
  {11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25};
static uint8_t brain_is_set = 0;

#define QUERY_STATE_URI_LEN  256
#define QUERY_STATE_PAYLOAD_LEN  256
#define QUERY_STATE_DATA_BUF_LEN  1024

#define IMPORTANT_MESSAGE_MAX_MAC_TRANSMISSIONS 3
#define NORMAL_MESSAGE_MAX_MAC_TRANSMISSIONS 1

#define DISCOVERY_STRING "NBR?"

typedef enum heartbeat_state_e {
  HEARTBEAT_GOOD,
  HEARTBEAT_BAD
} heartbeat_state_t;
#define HEARTBEAT_STRING "********************************"
#define HEARTBEAT_TIMEOUT CLOCK_SECOND/10
#define HEARTBEAT_GOOD_COUNT 3
#define HEARTBEAT_BAD_COUNT 2
#define HEARTBEAT_DISABLE_TIME CLOCK_SECOND
static heartbeat_state_t heartbeat_state = HEARTBEAT_BAD;
static int heartbeat_count = HEARTBEAT_BAD_COUNT;
static uint8_t heartbeat_enabled = 1;
static uint8_t heartbeat_msg_id = 0;
static uint8_t heartbeat_response_sent = 0;
/*---------------------------------------------------------------------------*/
typedef struct query_state_s {
  uint8_t type;
  uint8_t status;
  uint16_t id;
  uint16_t dataLen;
  uint8_t uri[QUERY_STATE_URI_LEN];
  uint8_t payload[QUERY_STATE_PAYLOAD_LEN];
  uint8_t data[QUERY_STATE_DATA_BUF_LEN];
} query_state_t;

typedef struct cp6_s {
  uip_ipaddr_t addr;
  uint8_t channel;
} cp6_t;

/*---------------------------------------------------------------------------*/
static uip_ipaddr_t brain_address, root_address;
static query_state_t query;
static msg_t msg_coap_ack;
static struct etimer query_timeout_timer;
static cp6_t cp6list[CP6_LIST_MAX];
static int cp6_count;
static struct ctimer heartbeat_timeout_ctimer;
/*---------------------------------------------------------------------------*/
PROCESS(query_process, "Query process");
PROCESS(config_process, "Configuration process");
PROCESS(discover_process, "Discover process");
PROCESS(coap_process, "CoAP process");
AUTOSTART_PROCESSES(&query_process);
/*---------------------------------------------------------------------------*/
void
print_timestamp(void)
{
  rtc_t timestamp;
  rtc_get_time(&timestamp);
  printf("[%02lu:%02lu:%02lu.%03lu] ", timestamp.hh, timestamp.mm, timestamp.ss, timestamp.ms);
}
/*---------------------------------------------------------------------------*/
char *strnstr(const char *haystack, const char *needle, size_t len)
{
  int i;
  size_t needle_len;

  /* segfault here if needle is not NULL terminated */
  if (0 == (needle_len = strlen(needle)))
    return (char *)haystack;

  for (i=0; i<=(int)(len-needle_len); i++)
  {
    if ((haystack[0] == needle[0]) && (0 == strncmp(haystack, needle, needle_len)))
      return (char *)haystack;

    haystack++;
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
print_addr(const uip_ipaddr_t *addr, char* buf, uint32_t* len)
{
  char *wr_ptr = buf;
  uint8_t ret;

  if(addr == NULL || addr->u8 == NULL) {
    *len = 0;
    return;
  }
  uint16_t a;
  unsigned int i;
  int f;
  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0) {
        strncpy(wr_ptr, "::", 2);
        wr_ptr += 2;
      }
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0) {
        strncpy(wr_ptr, ":", 1);
        wr_ptr += 1;
      }
      ret = sprintf(wr_ptr, "%x", a);
      wr_ptr += ret;
    }
  }
  wr_ptr+=1;
  *wr_ptr = 0;
  *len = wr_ptr - buf;
}

static int get_rf_channel(void)
{
  radio_value_t chan;
  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &chan);
  return (int)chan;
}

static int set_rf_channel(radio_value_t ch)
{
  if(ch < FIRST_CHANNEL || ch > LAST_CHANNEL){
    printf("Not a valid channel: %u\n", ch);
    return -1;
  }
  printf("Setting new channel: %u\n", ch);
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, ch);
  return 0;
}

static void heartbeat_enable(void *p_data){
  heartbeat_enabled = 1;
}
static void heartbeat_disable(){
  heartbeat_enabled = 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(query_process, ev, data)
{
  msg_t* msg;
  msg_t msg_resp;
  uint8_t resp[16];

  PROCESS_BEGIN();
  INFOT("INIT: Starting query process\n");
  INFOT("INIT: Version: %s.%s compiled @ %s %s)\n", FW_MAJOR_VERSION, version, __DATE__, __TIME__);

  process_start(&config_process, NULL);
  process_start(&serial_parser_process, NULL);
  process_start(&rest_server_process, NULL);
  process_start(&discover_process, NULL);
  process_start(&coap_process, NULL);

  uip_ds6_init();
  uip_nd6_init();
  rpl_init();

  while(1) {
    PROCESS_YIELD();
    if(ev == PROCESS_EVENT_MSG)
    {
      memset(resp, 0x20, 16);
      resp[15] = 0;
      msg = (msg_t*)data;
      if(msg_resp.type)
      {
        msg_resp.id = msg->id;
        msg_resp.len = 16;
        msg_resp.data = resp;
        send_msg(&msg_resp);
      }
    }
    else if(ev == PROCESS_EVENT_TIMER)
    {
      etimer_stop(&query_timeout_timer);
      // if(query.flags & QSF_PROCESSING)
      // {
      //   // send message
      //   INFOT("Query: query timed out!\n");
      //   query.flags = QSF_IDLE;
      //   msg_resp.type = 0xee;
      //   msg_resp.id = query.id;
      //   memset(resp, 0x20, 16);
      //   memcpy(resp, "TIMEOUT", 7);
      //   msg_resp.data = resp;
      //   msg_resp.len = 16;
      //   send_msg(&msg_resp);
      // }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
coap_chunk_handler(void *response)
{
  const uint8_t *chunk;
  int i;
  int len = coap_get_payload(response, &chunk);

  memcpy(query.data + query.dataLen, chunk, len);
  query.dataLen += len;
  query.status = ((coap_packet_t*)response)->code;

  INFOT("Chunk (len: %u: code: %u)", len, query.status);
  for(i=0; i<len; i++)
    INFO("%c", chunk[i]);
  INFO("#\n");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coap_process, ev, data)
{
  static coap_packet_t request[1];
  static msg_t* ptrMsg;
  char* newLine;
  static struct ctimer heartbeat_disable_ctimer;

  PROCESS_BEGIN();

  coap_init_engine();

  while(1) {
    PROCESS_YIELD();
    if(ev == PROCESS_EVENT_MSG) {
      ptrMsg = (msg_t*)data;
      query.type = ptrMsg->type;
      query.id = ptrMsg->id;
      query.status = 0;
      query.dataLen = 0;
      memset(query.uri, 0, QUERY_STATE_URI_LEN);
      memset(query.payload, 0, QUERY_STATE_PAYLOAD_LEN);

      newLine = strnstr(ptrMsg->data, "\n", ptrMsg->len);
      if(newLine) {
        INFOT("New line found at %u\n", newLine - (char*)ptrMsg->data);
        strncpy(query.uri, ptrMsg->data, newLine - (char*)ptrMsg->data);
        strncpy(query.payload, newLine + 1, ptrMsg->len - (newLine - (char*)ptrMsg->data) - 1);
      }
      else {
        strncpy(query.uri, ptrMsg->data, ptrMsg->len);
      }
      INFOT("URI(%u): %s\n", strlen(query.uri), query.uri);
      INFOT("Payload(%u): %s\n", strlen(query.payload), query.payload);

      if(ptrMsg->type == T_REST_GET) {
        coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
        INFOT("GET URI: %s\n", query.uri);
      }
      else if(ptrMsg->type == T_REST_POST)
      {
        coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
        coap_set_payload(request, (uint8_t *)query.payload, strlen(query.payload));
      }
      else if(ptrMsg->type == T_REST_PUT)
      {
        coap_init_message(request, COAP_TYPE_CON, COAP_PUT, 0);
        coap_set_payload(request, (uint8_t *)query.payload, strlen(query.payload));
      }
      else if(ptrMsg->type == T_REST_DELETE)
      {
        coap_init_message(request, COAP_TYPE_CON, COAP_DELETE, 0);
      }

      /* disable heartbeat for HEARTBEAT_DISABLE_TIME */
      heartbeat_disable();
      ctimer_set(&heartbeat_disable_ctimer, HEARTBEAT_DISABLE_TIME, heartbeat_enable, NULL);

      if(ptrMsg->type == T_TRIGGER_ACTION){
        //send non confirmable message
        coap_init_message(request, COAP_TYPE_NON, COAP_GET, 0);
        coap_set_header_uri_path(request, query.uri);
        coap_set_header_block2(request, 0, 0, REST_MAX_CHUNK_SIZE);
        request->mid = coap_get_mid();
        coap_transaction_t *tmpTransaction =  coap_new_transaction(request->mid, &brain_address, BRAIN_COAP_PORT);
        if(tmpTransaction) {
          tmpTransaction->packet_len = coap_serialize_message(request, tmpTransaction->packet);
          sicslowpan_set_max_mac_transmissions(NORMAL_MESSAGE_MAX_MAC_TRANSMISSIONS);
          coap_send_transaction(tmpTransaction);
          sicslowpan_reset_max_mac_transmissions();
          msg_coap_ack.id = query.id;
          msg_coap_ack.type = query.type;
          msg_coap_ack.len = 0;
          msg_coap_ack.data = NULL;
        }
      }
      else {
        //send confirmable message - make sure we include a token as the answer of the server is returned
        //as a separate message (not piggybacked).
        coap_set_header_uri_path(request, query.uri);
        coap_set_header_block2(request, 0, 0, REST_MAX_CHUNK_SIZE);
        unsigned int random = random_rand();
        unsigned char token[2];
        token[0] = random & 0xff;
        token[1] = (random>>8) & 0xff;
        coap_set_token(request, token, 2);
        sicslowpan_set_max_mac_transmissions(IMPORTANT_MESSAGE_MAX_MAC_TRANSMISSIONS);
        COAP_BLOCKING_REQUEST(&brain_address, BRAIN_COAP_PORT, request, coap_chunk_handler);
        sicslowpan_reset_max_mac_transmissions();
        msg_coap_ack.id = query.id;
        msg_coap_ack.type = query.type;
        msg_coap_ack.len = query.dataLen;
        msg_coap_ack.data = query.data;
        INFOT("Response status: %u", query.status);
      }
      send_msg(&msg_coap_ack);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void heartbeat_send_msg(uint8_t id){
  if(!heartbeat_response_sent){
    heartbeat_response_sent = 1;
    msg_t heartbeat_msg;
    uint8_t response;
    if(heartbeat_state == HEARTBEAT_GOOD && heartbeat_count <= 0){
      heartbeat_state = HEARTBEAT_BAD;
      heartbeat_count = 0;
    }
    else if(heartbeat_state == HEARTBEAT_BAD && heartbeat_count >= HEARTBEAT_GOOD_COUNT){
      heartbeat_state = HEARTBEAT_GOOD;
      heartbeat_count = HEARTBEAT_BAD_COUNT;
    }

    if(heartbeat_state == HEARTBEAT_GOOD)
      response = 1;
    else
      response = 0;
    INFOT("T_HEARTBEAT: %d\n", response);
    heartbeat_msg.id = id;
    heartbeat_msg.type = T_HEARTBEAT;
    heartbeat_msg.len = 1;
    heartbeat_msg.data = &response;
    send_msg(&heartbeat_msg);
  }
}

static void heartbeat_callback(struct simple_udp_connection *c,
                       const uip_ipaddr_t *source_addr,
                       uint16_t source_port,
                       const uip_ipaddr_t *dest_addr,
                       uint16_t dest_port,
                       const uint8_t *data, uint16_t datalen){
  if(datalen == strlen(HEARTBEAT_STRING)){
    if(memcmp(HEARTBEAT_STRING, data, datalen) == 0){
      ctimer_stop(&heartbeat_timeout_ctimer);
      if(heartbeat_state == HEARTBEAT_GOOD)
        heartbeat_count = HEARTBEAT_BAD_COUNT;
      else
        ++heartbeat_count;
    }

    heartbeat_send_msg(heartbeat_msg_id);
  }
}

static void heartbeat_timeout_callback(void *p_data){
  if(heartbeat_state == HEARTBEAT_GOOD)
    --heartbeat_count;
  else
    heartbeat_count = 0;
  heartbeat_send_msg(heartbeat_msg_id);
}

PROCESS_THREAD(config_process, ev, data)
{
  msg_t* msg_ptr;
  static msg_t msg_buf;
  char resp_buf[40];
  char addr_buf[48];
  uint32_t addr_buf_len = 48;
  radio_value_t rssi_val = 0;
  static struct simple_udp_connection hearbeat_conn;
  static struct etimer et;

  PROCESS_BEGIN();
  INFOT("INIT: Starting config process\n");
  simple_udp_register(&hearbeat_conn,
                      3001,
                      NULL,
                      3200,
                      heartbeat_callback);
  while(1) {
    PROCESS_YIELD();
    if(ev == PROCESS_EVENT_MSG)
    {
      memset(resp_buf, 0, sizeof(resp_buf));
      memset(addr_buf, 0, sizeof(addr_buf));
      msg_ptr = (msg_t*)data;
      msg_buf.type = msg_ptr->type;
      msg_buf.id = msg_ptr->id;
      if(msg_ptr->type == T_STATUS)
      {
        NETSTACK_CONF_RADIO.get_value(RADIO_PARAM_RSSI, &rssi_val);
        if(rssi_val == 0)
          resp_buf[0] = '0';
        else if(rssi_val > 0)
          resp_buf[0] = '4';
        else if((rssi_val > -9) && (rssi_val < 0))
          resp_buf[0] = '3';
        else if((rssi_val > -19) && (rssi_val < -10))
          resp_buf[0] = '2';
        else if((rssi_val > -29) && (rssi_val < -20))
          resp_buf[0] = '1';
        else
          resp_buf[0] = '0';

        msg_buf.data = (uint8_t*)resp_buf;
        msg_buf.len = 1;
        send_msg(&msg_buf);
      }
      else if(msg_ptr->type == T_BRAIN_INFO)
      {
        uint32_t addr_len = 0;
        print_addr(&brain_address, resp_buf, &addr_len);
        msg_buf.data = (uint8_t*)resp_buf;
        msg_buf.len = addr_len;
        send_msg(&msg_buf);
      }
      else if(msg_ptr->type == T_PAIR_WITH_CP6)
      {
        uip_ipaddr_t tmp_address;
        if(uiplib_ipaddrconv((const char *)msg_ptr->data, &tmp_address)){
          uip_ipaddr_copy(&brain_address, &tmp_address);
          msg_buf.data = NULL;
          msg_buf.len = 0;
          brain_is_set = 1;
        }
        else {
        	strcpy(resp_buf, "Not valid address!");
        	msg_buf.type = T_ERROR_RESPONSE;
        	msg_buf.data = (uint8_t*)resp_buf;
        	msg_buf.len = strlen(resp_buf) + 1;
          brain_is_set = 0;
        }
        send_msg(&msg_buf);

        if(brain_is_set){
          rpl_dag_t *rpl_current_dag = rpl_get_any_dag();
          if(rpl_current_dag != NULL)
            rpl_free_instance(rpl_current_dag->instance);

          etimer_set(&et, CLOCK_SECOND);
          process_post_synch(&discover_process, PROCESS_EVENT_MSG, &msg_buf);
        }

        print_addr(&brain_address, addr_buf, &addr_buf_len);
        INFOT("INIT: Brain address: %s\n", addr_buf);
      }
      else if(msg_ptr->type == T_GET_FW_VERSION)
      {
        strcpy(resp_buf, FW_MAJOR_VERSION);
        msg_buf.data = (uint8_t*)resp_buf;
        msg_buf.len = strlen(FW_MAJOR_VERSION) + 1;
        send_msg(&msg_buf);
      }
      else if(msg_ptr->type == T_GET_LOCAL_IP)
      {
        uint32_t addr_len = 0;
        uip_ds6_addr_t* ipaddr;
        ipaddr = uip_ds6_get_global(ADDR_PREFERRED);
        if(ipaddr == NULL)
        {
          strcpy(resp_buf, "Address not set");
          msg_buf.type = T_ERROR_RESPONSE;
          msg_buf.data = (uint8_t*)resp_buf;
          msg_buf.len = strlen(resp_buf) + 1;
        }
        else
        {
          print_addr(&ipaddr->ipaddr, resp_buf, &addr_len);
          msg_buf.data = (uint8_t*)resp_buf;
          msg_buf.len = addr_len;
        }
        send_msg(&msg_buf);
      }
      else if(msg_ptr->type == T_HEARTBEAT){
        heartbeat_response_sent = 0;
        if(brain_is_set && rpl_get_any_dag()){
          if(heartbeat_enabled){
            heartbeat_msg_id = msg_buf.id;
            char heartbeat_buf[64];
            strcpy(heartbeat_buf, HEARTBEAT_STRING);
            sicslowpan_set_max_mac_transmissions(NORMAL_MESSAGE_MAX_MAC_TRANSMISSIONS);
            simple_udp_sendto(&hearbeat_conn, heartbeat_buf, strlen(HEARTBEAT_STRING), &root_address);
            sicslowpan_reset_max_mac_transmissions();
            ctimer_set(&heartbeat_timeout_ctimer, HEARTBEAT_TIMEOUT, heartbeat_timeout_callback, NULL);
          }
          else
            heartbeat_send_msg(msg_buf.id);
        }
        else {
          INFOT("T_HEARTBEAT: not connected\n");
          resp_buf[0] = 0;
          msg_buf.data = (uint8_t*)resp_buf;
          msg_buf.len = 1;
          send_msg(&msg_buf);
        }
      }
    }
    else if(ev == PROCESS_EVENT_TIMER)
    {
      rpl_dag_t *rpl_current_dag = rpl_get_any_dag();
      if(rpl_current_dag == NULL){
        INFOT("Waiting to join a network\n");
        dis_output(NULL);
        etimer_set(&et, CLOCK_SECOND);
      }
      else if(memcmp(&rpl_current_dag->dag_id, &brain_address, 8) != 0){
        INFOT("Joined the wrong network!\n");
        rpl_free_instance(rpl_current_dag->instance);
        // send DIS to join new network
        dis_output(NULL);
        etimer_set(&et, CLOCK_SECOND);
      }
      else{
        INFOT("Joined network!\n");
        root_address = rpl_current_dag->dag_id;
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void discover_callback(struct simple_udp_connection *c,
                       const uip_ipaddr_t *source_addr,
                       uint16_t source_port,
                       const uip_ipaddr_t *dest_addr,
                       uint16_t dest_port,
                       const uint8_t *data, uint16_t datalen)
{
  uint32_t buf_len = 64;
  char buf[64];
  int i;

  print_addr(source_addr, buf, &buf_len);
  INFOT("DISCOVER: from: %s msg: ", buf);
  for(i = 0; i<datalen; i++)
    INFO("%c", data[i]);
  INFO("\n");

  if(cp6_count < CP6_LIST_MAX){
    ++cp6_count;
    uiplib_ipaddrconv((const char*)&data[2], &cp6list[cp6_count-1].addr); // ignore data[0]='Y', data[1]=' '
    cp6list[cp6_count-1].channel = (uint8_t) get_rf_channel();
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(discover_process, ev, data)
{
  static struct simple_udp_connection client_conn;
  static uip_ipaddr_t addr;
  static msg_t msg_buf;
  static struct etimer discover_timeout;
  msg_t* msg_ptr;
  char buf[128];
  uint32_t buf_len = 0;
  static int current_channel_index = 0;
  static uint8_t brain_found = 0;
  PROCESS_BEGIN();

  simple_udp_register(&client_conn,
                      3000,
                      NULL,
                      3200,
                      discover_callback);

  while(1) {
    PROCESS_YIELD();
    if(ev == PROCESS_EVENT_MSG)
    {
      etimer_stop(&discover_timeout);
      cp6_count = 0;
      current_channel_index = 0;
      brain_found = 0;
      if(data != NULL){
        msg_ptr = (msg_t*)data;
        msg_buf.type = msg_ptr->type;
        msg_buf.id = msg_ptr->id;
      }

      set_rf_channel(operating_channels[current_channel_index]);
      uip_create_linklocal_allnodes_mcast(&addr);
      print_addr(&addr, buf, &buf_len);
      INFOT("DISCOVER: sending whois to %s on channel %d\n", buf, operating_channels[current_channel_index]);
      memcpy(buf, DISCOVERY_STRING, strlen(DISCOVERY_STRING));
      simple_udp_sendto(&client_conn, buf, strlen(DISCOVERY_STRING), &addr);

      etimer_set(&discover_timeout, DISCOVERY_DUTY_CYCLE);
    } else if(ev == PROCESS_EVENT_TIMER) {
      ++current_channel_index;
      if(msg_buf.type == T_PAIR_WITH_CP6){
        int i;
        for(i = 0; i < cp6_count; i++){
          if(uip_ipaddr_cmp(&cp6list[i].addr, &brain_address)){
            set_rf_channel(cp6list[i].channel);
            brain_found = 1;
            break;
          }
        }
      }

      if(current_channel_index < OPERATING_CHANNELS && brain_found == 0){
        set_rf_channel(operating_channels[current_channel_index]);
        uip_create_linklocal_allnodes_mcast(&addr);
        print_addr(&addr, buf, &buf_len);
        INFOT("DISCOVER: sending whois to %s on channel %d\n", buf, operating_channels[current_channel_index]);
        memcpy(buf, DISCOVERY_STRING, strlen(DISCOVERY_STRING));
        simple_udp_sendto(&client_conn, buf, strlen(DISCOVERY_STRING), &addr);
        etimer_set(&discover_timeout, DISCOVERY_DUTY_CYCLE);
      }
      else if (msg_buf.type == T_GET_CP6_LIST){
        buf_len = 0;
        uint32_t addr_len;
        int i;
        for(i = 0; i < cp6_count; i++){
          addr_len = 0;
          print_addr(&cp6list[i].addr, buf+buf_len, &addr_len);
          buf_len += addr_len;
          buf[buf_len++] = '\n';
        }

        buf[buf_len-1] = '\0';
        msg_buf.data = (unsigned char*)buf;
        msg_buf.len = buf_len;
        send_msg(&msg_buf);
      }

    }
  }
  PROCESS_END();
}
