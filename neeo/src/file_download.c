
/*
 * main.c - file download sample application
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "msleep.h"
#include <stdint.h>
#include "file_download.h"
#include "simplelink.h"
#include "sl_common.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#undef uint32_t

#if 0
   #define debug_printf( __fmt, ... ) printf("(%s:%s:%d): " __fmt, __FILE__, __FUNCTION__, __LINE__,## __VA_ARGS__ )
#else
   #define debug_printf(args...)
#endif

#define SL_STOP_TIMEOUT        0xFF

// GET /symbol.dat HTTP/1.1
// User-Agent: Wget/1.16 (linux-gnu)
// Accept: */*
// Host: 192.168.1.80
// Connection: Keep-Alive

#define POST_REQUEST   "GET /" /* GET <filename>\r\n */
#define POST_PROTOCOL   " HTTP/1.1\r\n"
#define POST_HOST       "Host: "
#define POST_HOST_2       "\r\nConnection: Keep-Alive\r\nAccept: */*\r\nUser-Agent: CC3100 (STM32)\r\n\r\n"

#define HTTP_FILE_NOT_FOUND    "404 Not Found" /* HTTP file not found response */
#define HTTP_STATUS_OK         "200 OK"  /* HTTP status ok response */
#define HTTP_CONTENT_LENGTH    "Content-Length:"  /* HTTP content length header */
#define HTTP_TRANSFER_ENCODING "Transfer-Encoding:" /* HTTP transfer encoding header */
#define HTTP_ENCODING_CHUNKED  "chunked" /* HTTP transfer encoding header value */
#define HTTP_CONNECTION        "Connection:" /* HTTP Connection header */
#define HTTP_CONNECTION_CLOSE  "close"  /* HTTP Connection header value */

#define HTTP_END_OF_HEADER  "\r\n\r\n"  /* string marking the end of headers in response */
#define HTTP_END_OF_LINE "\r\n"

#define READ_SIZE       1450
#define MAX_BUFF_SIZE   1460
#define SPACE           32

_u32 content_length = 0;

/* Application specific status/error codes */
typedef enum{
    DEVICE_NOT_IN_STATION_MODE = -0x7D0,        /* Choosing this number to avoid overlap with host-driver's error codes */
    INVALID_HEX_STRING = DEVICE_NOT_IN_STATION_MODE - 1,
    TCP_RECV_ERROR = INVALID_HEX_STRING - 1,
    TCP_SEND_ERROR = TCP_RECV_ERROR - 1,
    FILE_NOT_FOUND_ERROR = TCP_SEND_ERROR - 1,
    INVALID_SERVER_RESPONSE = FILE_NOT_FOUND_ERROR - 1,
    FORMAT_NOT_SUPPORTED = INVALID_SERVER_RESPONSE - 1,
    FILE_WRITE_ERROR = FORMAT_NOT_SUPPORTED - 1,
    INVALID_FILE = FILE_WRITE_ERROR - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

/*
 * GLOBAL VARIABLES -- Start
 */
_u32 g_Status;
_u32 g_DestinationIP;
_u32 g_BytesReceived; /* variable to store the file size */
_u8  g_buff[MAX_BUFF_SIZE+1];

_i32 g_SockID = 0;
_u8* FileMem = 0;

/*
 * GLOBAL VARIABLES -- End
 */


/*
 * STATIC FUNCTION DEFINITIONS -- Start
 */
static _i32 establishConnectionWithAP(uint8_t * ssid, int ssid_length, uint8_t * pass, int pass_length);
static _i32 configureSimpleLinkToDefaultState();

static _i32 initializeAppVariables();

static _i32 createConnection(_u32 DestinationIP);

static _i32 getChunkSize(_i32 *len, _u8 **p_Buff, _u32 *chunk_size);
static _i32 hexToi(_u8 *ptr);
static _i32 getFile(const char* host_ip, unsigned int* size, _u8* file);
/*
 * STATIC FUNCTION DEFINITIONS -- End
 */


/*
 * ASYNCHRONOUS EVENT HANDLERS -- Start
 */

/*!
    \brief This function handles WLAN events

    \param[in]      pWlanEvent is the event passed to the handler

    \return         None

    \note

    \warning
*/
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    if(pWlanEvent == NULL)
        debug_printf("[WLAN EVENT] NULL Pointer Error \n\r");
    
    switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {
            SET_STATUS_BIT(g_Status, STATUS_BIT_CONNECTION);

            /*
             * Information about the connected AP (like name, MAC etc) will be
             * available in 'slWlanConnectAsyncResponse_t' - Applications
             * can use it if required
             *
             * slWlanConnectAsyncResponse_t *pEventData = NULL;
             * pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
             *
             */
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_Status, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_Status, STATUS_BIT_IP_ACQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            /* If the user has initiated 'Disconnect' request, 'reason_code' is SL_USER_INITIATED_DISCONNECTION */
            if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                debug_printf(" Device disconnected from the AP on application's request \n\r");
            }
            else
            {
                debug_printf(" Device disconnected from the AP, reason 0x%08x..!! \n\r", pEventData->reason_code);
            }
        }
        break;

        default:
        {
            debug_printf(" [WLAN EVENT] Unexpected event \n\r");
        }
        break;
    }
}

/*!
    \brief This function handles events for IP address acquisition via DHCP
           indication

    \param[in]      pNetAppEvent is the event passed to the handler

    \return         None

    \note

    \warning
*/
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    if(pNetAppEvent == NULL)
        debug_printf(" [NETAPP EVENT] NULL Pointer Error \n\r");
 
    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SET_STATUS_BIT(g_Status, STATUS_BIT_IP_ACQUIRED);

            /*
             * Information about the connected AP's IP, gateway, DNS etc
             * will be available in 'SlIpV4AcquiredAsync_t' - Applications
             * can use it if required
             *
             * SlIpV4AcquiredAsync_t *pEventData = NULL;
             * pEventData = &pNetAppEvent->EventData.ipAcquiredV4;
             * <gateway_ip> = pEventData->gateway;
             *
             */
        }
        break;

        default:
        {
            debug_printf(" [NETAPP EVENT] Unexpected event \n\r");
        }
        break;
    }
}

/*!
    \brief This function handles socket events indication

    \param[in]      pSock is the event passed to the handler

    \return         None
*/
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    if(pSock == NULL)
        debug_printf(" [SOCK EVENT] NULL Pointer Error \n\r");
    
    switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
        {
            /*
            * TX Failed
            *
            * Information about the socket descriptor and status will be
            * available in 'SlSockEventData_t' - Applications can use it if
            * required
            *
            * SlSockEventData_t *pEventData = NULL;
            * pEventData = & pSock->EventData;
            */
            switch( pSock->socketAsyncEvent.SockTxFailData.status)
            {
                case SL_ECLOSE:
                    debug_printf(" [SOCK EVENT] Close socket operation failed to transmit all queued packets\n\r");
                break;


                default:
                    debug_printf(" [SOCK EVENT] Unexpected event \n\r");
                break;
            }
        }
        break;

        default:
            debug_printf(" [SOCK EVENT] Unexpected event \n\r");
        break;
    }
}
/*
 * ASYNCHRONOUS EVENT HANDLERS -- End
 */

cyg_uint32 wifi_init(uint8_t * ssid, int ssid_length, uint8_t * pass, int pass_length)
{
    _i32 retVal = -1;

    retVal = initializeAppVariables();
    ASSERT_ON_ERROR(retVal);
    debug_printf("Start!\n\r");
    /* Stop WDT and initialize the system-clock of the MCU */
    stopWDT();
    initClk();
    //printf("Early start ok...\n\r");

    /*
     * Following function configures the device to default state by cleaning
     * the persistent settings stored in NVMEM (viz. connection profiles &
     * policies, power policy etc)
     *
     * Applications may choose to skip this step if the developer is sure
     * that the device is in its default state at start of application
     *
     * Note that all profiles and persistent settings that were done on the
     * device will be lost
     */
    retVal = configureSimpleLinkToDefaultState();
    if(retVal < 0)
    {
        if (DEVICE_NOT_IN_STATION_MODE == retVal)
            debug_printf(" Failed to configure the device in its default state \n\r");

        return -1;
    }

    //TODO check if this is neccessary
    msleep(1);


    /*
     * Initializing the CC3100 device
     * Assumption is that the device is configured in station mode already
     * and it is in its default state
     */
    retVal = sl_Start(0, 0, 0);
    if ((retVal < 0) ||
        (ROLE_STA != retVal) )
    {
        debug_printf(" Failed to start the device \n\r");
        return -1;
    }

    debug_printf(" Device started as STATION \n\r");

    /* Connecting to WLAN AP */
    retVal = establishConnectionWithAP(ssid, ssid_length, pass, pass_length);
    if(retVal < 0)
    {
        debug_printf(" Failed to establish connection w/ an AP retVal = %lu\n\r", retVal);
        return -1;
    }

    debug_printf(" Connection established w/ AP and IP is acquired \n\r");

    return 0;
}

cyg_uint32 wifi_deinit(void)
{
    _i32 retVal = -1;
    /* Stop the CC3100 device */
    retVal = sl_Stop(SL_STOP_TIMEOUT);
    if(retVal < 0)
    {
        debug_printf(" Device couldn't be stopped \r\n");
        return -1;
    }

    return 0;
}

int wifi_download(const char* name, const char* host_ip, unsigned int* size, unsigned char** content)
{
    _i32 retVal = -1;
    //SlFsFileInfo_t file_info;
    //_i32 file_handle = -1;
    *size = 0;

    retVal = sl_NetAppDnsGetHostByName((signed char*)host_ip, pal_Strlen(host_ip), &g_DestinationIP, SL_AF_INET);
    if(retVal < 0)
    {
        debug_printf(" Device couldn't get the IP for the host-name: %s\r\n", host_ip);
        return -1;
    }

    /* Create a TCP connection to the Web Server */
    g_SockID = createConnection(g_DestinationIP);
    if(g_SockID < 0)
    {
        debug_printf(" Device couldn't connect to the server\r\n");
        return -1;
    }

    debug_printf(" Successfully connected to the server \r\n");

    /* Download the file, verify the file and replace the exiting file */
    retVal = getFile(host_ip, size, (unsigned char*)name);
    if(retVal < 0)
    {
        debug_printf(" Device couldn't download the file from the server \r\n");
        return -1;
    } else { // everything ok
        *content = FileMem;
        debug_printf(" File downloaded successfully %p %p \r\n", *content, FileMem );
    }

    retVal = sl_Close(g_SockID);
    if(retVal < 0)
    {
        debug_printf(" Socket couldn't be closed\r\n");
        return -1;
    }

    return 0;
}

/*!
    \brief Create connection with server.

    This function opens a socket and create the endpoint communication with server

    \param[in]      DestinationIP - IP address of the server

    \return         socket id for success and negative for error
*/
static _i32 createConnection(_u32 DestinationIP)
{
    SlSockAddrIn_t  Addr = {0};
    _i32           Status = 0;
    _i32           AddrSize = 0;
    _i32           SockID = 0;

    Addr.sin_family = SL_AF_INET;
    Addr.sin_port = sl_Htons(HOST_PORT);
    Addr.sin_addr.s_addr = sl_Htonl(DestinationIP);

    AddrSize = sizeof(SlSockAddrIn_t);

    SockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
    ASSERT_ON_ERROR(SockID);

    Status = sl_Connect(SockID, ( SlSockAddr_t *)&Addr, AddrSize);
    if (Status < 0)
    {
        sl_Close(SockID);
        ASSERT_ON_ERROR(Status);
    }

    return SockID;
}

/*!
    \brief Convert hex to decimal base

    \param[in]      ptr - pointer to string containing number in hex

    \return         number in decimal base

*/
static _i32 hexToi(_u8 *ptr)
{
    _u32 result = 0;
    _u32 len = 0;

    _i32  idx = -1;

    len = pal_Strlen(ptr);

    /* convert characters to upper case */
    for(idx = 0; ptr[idx] != '\0'; ++idx)
    {
        if( (ptr[idx] >= 'a') &&
            (ptr[idx] <= 'f') )
        {
            ptr[idx] -= 32;         /* Change case - ASCII 'a' = 97, 'A' = 65 => 97-65 = 32 */
        }
    }

    for(idx = 0; ptr[idx] != '\0'; ++idx)
    {
        if(ptr[idx] >= '0' && ptr[idx] <= '9')
        {
            /* Converting '0' to '9' to their decimal value */
            result += (ptr[idx] - '0') * (1 << (4 * (len - 1 - idx)));
        }
        else if(ptr[idx] >= 'A' && ptr[idx] <= 'F')
        {
            /* Converting hex 'A' to 'F' to their decimal value */
            result += (ptr[idx] - 55) * (1 << (4 * (len -1 - idx))); /* .i.e. 'A' - 55 = 10, 'F' - 55 = 15 */
        }
        else
        {
            ASSERT_ON_ERROR(INVALID_HEX_STRING);
        }
    }

    return result;
}

/*!
    \brief Calculate the file chunk size

    \param[in]      len - pointer to length of the data in the buffer
    \param[in]      p_Buff - pointer to pointer of buffer containing data
    \param[out]     chunk_size - pointer to variable containing chunk size

    \return         0 for success, -ve for error

*/
static _i32 getChunkSize(_i32 *len, _u8 **p_Buff, _u32 *chunk_size)
{
    _i32   idx = -1;
    _u8   lenBuff[10];

    idx = 0;
    pal_Memset(lenBuff, 0, sizeof(lenBuff));
    while(*len >= 0 && **p_Buff != 13) /* check for <CR> */
    {
        if(0 == *len)
        {
            pal_Memset(g_buff, 0, sizeof(g_buff));
            *len = sl_Recv(g_SockID, &g_buff[0], MAX_BUFF_SIZE, 0);
            if(*len <= 0)
                ASSERT_ON_ERROR(TCP_RECV_ERROR);

            *p_Buff = g_buff;
        }

        lenBuff[idx] = **p_Buff;
        idx++;
        (*p_Buff)++;
        (*len)--;
    }

    (*p_Buff) += 2; /* skip <CR><LF> */
    (*len) -= 2;
    *chunk_size = hexToi(lenBuff);

    return SUCCESS;
}

/*!
    \brief Obtain the file from the server

    This function requests the file from the server and save it on serial flash.
    To request a different file for different user needs to modify the
    PREFIX_BUFFER and POST_BUFFER macros.

    \param[in]      None

    \return         0 for success and negative for error

*/
static _i32 getFile(const char* host_ip, unsigned int* size, _u8* file)
{
//    _u32 Token = 0;
    _u32 recv_size = 0;
    content_length=0;
    _u8 *pBuff = 0;
//    _u8 eof_detected = 0;
    _u8 isChunked = 0;

    g_BytesReceived = 0;
    *FileMem = 0;

    _i32 transfer_len = -1;
    _i32 retVal = -1;
//    _i32 fileHandle = -1;
    pal_Memset(g_buff, 0, sizeof(g_buff));

    /*Puts together the HTTP GET string.*/
    pal_Strcat(g_buff, POST_REQUEST);
    pal_Strcat(g_buff, file);
    pal_Strcat(g_buff, POST_PROTOCOL);
    pal_Strcat(g_buff, POST_HOST);
    pal_Strcat(g_buff, host_ip);
    pal_Strcat(g_buff, POST_HOST_2);

    /*Send the HTTP GET string to the opened TCP/IP socket.*/
    transfer_len = sl_Send(g_SockID, g_buff, pal_Strlen(g_buff), 0);
    if (transfer_len < 0)
    {
        /* error */
        debug_printf(" Socket Send Error\r\n");
        ASSERT_ON_ERROR(TCP_SEND_ERROR);
    }
    pal_Memset(g_buff, 0, sizeof(g_buff));

    /*get the reply from the server in buffer.*/
    transfer_len = sl_Recv(g_SockID, &g_buff[0], MAX_BUFF_SIZE, 0);
    if(transfer_len <= 0)
        ASSERT_ON_ERROR(TCP_RECV_ERROR);
    debug_printf(" transfer_len = %ld\r\n", transfer_len);

    /* Check for 404 return code */
    if(pal_Strstr(g_buff, HTTP_FILE_NOT_FOUND) != 0)
    {
        debug_printf(" File not found, check the file and try again\r\n");
        ASSERT_ON_ERROR(FILE_NOT_FOUND_ERROR);
    }

    /* if not "200 OK" return error */
    if(pal_Strstr(g_buff, HTTP_STATUS_OK) == 0)
    {
        debug_printf(" Error during downloading the file\r\n");
        ASSERT_ON_ERROR(INVALID_SERVER_RESPONSE);
    }

    /* check if content length is transferred with headers */
    pBuff = (_u8 *)pal_Strstr(g_buff, HTTP_CONTENT_LENGTH);

    if(pBuff != 0)
    {
        /* we received Content-Length header, get its value, skip header name */
        pBuff += pal_Strlen(HTTP_CONTENT_LENGTH);
        /* skip whitespace between header name and its value */
        while(*pBuff == SPACE)
            pBuff++;

        content_length = 0;
        while((*pBuff != '\n') && (*pBuff != '\r'))
        {
            content_length *= 10;
            content_length += ((*pBuff) - '0');
            pBuff++;
        }

        FileMem = custom_malloc(content_length);
        *size = content_length;
        debug_printf(" Content-Length = %lu\n\r Allocating memory! %p \r\n", content_length, FileMem);
        if(FileMem == 0) return -1;
        memset(FileMem, 0, content_length);
    }

    /* Check if data is chunked */
    pBuff = (_u8 *)pal_Strstr(g_buff, HTTP_TRANSFER_ENCODING);
    if(pBuff != 0)
    {
        pBuff += pal_Strlen(HTTP_TRANSFER_ENCODING);
        while(*pBuff == SPACE)
            pBuff++;

        if(pal_Memcmp(pBuff, HTTP_ENCODING_CHUNKED, pal_Strlen(HTTP_ENCODING_CHUNKED)) == 0)
        {
            recv_size = 0;
            isChunked = 1;
        }
    }
    else
    {
        /* Check if connection will be closed by after sending data
         * In this method the content length is not received and end of
         * connection marks the end of data */
        pBuff = (_u8 *)pal_Strstr(g_buff, HTTP_CONNECTION);
        if(pBuff != 0)
        {
            pBuff += pal_Strlen(HTTP_CONNECTION);
            while(*pBuff == SPACE)
                pBuff++;

            if(pal_Memcmp(pBuff, HTTP_ENCODING_CHUNKED, pal_Strlen(HTTP_CONNECTION_CLOSE)) == 0)
            {
                /* not supported */
                debug_printf(" Server response format is not supported\r\n");
                ASSERT_ON_ERROR(FORMAT_NOT_SUPPORTED);
            }
        }
    }

    /* "\r\n\r\n" marks the end of headers */
    pBuff = (_u8 *)pal_Strstr(g_buff, HTTP_END_OF_HEADER);
    if(pBuff == 0)
    {
        debug_printf(" Invalid response\r\n");
        ASSERT_ON_ERROR(INVALID_SERVER_RESPONSE);
    }
    /* Increment by 4 to skip "\r\n\r\n" */
    pBuff += 4;

    /* Adjust buffer data length for header size */
    transfer_len -= (pBuff - g_buff);

    /* If data in chunked format, calculate the chunk size */
    if(isChunked == 1)
    {
        retVal = getChunkSize(&transfer_len, &pBuff, &recv_size);
        if(retVal < 0)
        {
            /* Error */
            debug_printf(" Problem with connection to server\r\n");
            return retVal;
        }
    }
    else
    {
        recv_size = content_length;
    }

    while (0 < transfer_len && FileMem != 0)
    {
        /* For chunked data recv_size contains the chunk size to be received
         * while the transfer_len contains the data in the buffer */
        if(recv_size <= transfer_len)
        {
            memcpy((_u8 *)(FileMem+g_BytesReceived), (_u8 *)pBuff, recv_size);
            retVal = recv_size;

            transfer_len -= recv_size;
            g_BytesReceived +=recv_size;
            pBuff += recv_size;
            recv_size = 0;

            if(isChunked == 1)
            {
                /* if data in chunked format calculate next chunk size */
                pBuff += 2; /* 2 bytes for <CR> <LF> */
                transfer_len -= 2;

                if(getChunkSize(&transfer_len, &pBuff, &recv_size) < 0)
                {
                    /* Error */
                    break;
                }

                /* if next chunk size is zero we have received the complete file */
                if(recv_size == 0)
                {
//                    eof_detected = 1;
                    break;
                }

                if(recv_size < transfer_len)
                {
                    /* Code will enter this section if the new chunk size is less then
                     * then the transfer size. This will the last chunk of file received
                     */
                    memcpy((_u8 *)(FileMem+g_BytesReceived), (_u8 *)pBuff, recv_size);
                    retVal = recv_size;

                    transfer_len -= recv_size;
                    g_BytesReceived +=recv_size;
                    pBuff += recv_size;
                    recv_size = 0;

                    pBuff += 2; /* 2bytes for <CR> <LF> */
                    transfer_len -= 2;

                    /* Calculate the next chunk size, should be zero */
                    if(getChunkSize(&transfer_len, &pBuff, &recv_size) < 0)
                    {
                        /* Error */
                        break;
                    }

                    /* if next chunk size is non zero error */
                    if(recv_size != 0)
                    {
                        /* Error */
                        break;
                    }
//                    eof_detected = 1;
                    break;
                }
                else
                {
                    /* write data on the file */
                    memcpy((_u8 *)(FileMem+g_BytesReceived), (_u8 *)pBuff, transfer_len);
                    retVal = transfer_len;

                    recv_size -= transfer_len;
                    g_BytesReceived +=transfer_len;
                }
            }
            /* complete file received exit */
            if(recv_size == 0)
            {
//                eof_detected = 1;
                break;
            }
        }
        else
        {
            /* write data on the file */
            memcpy((_u8 *)(FileMem+g_BytesReceived), (_u8 *)pBuff, transfer_len);
            retVal = transfer_len;

            g_BytesReceived +=transfer_len;
            recv_size -= transfer_len;
        }

        pal_Memset(g_buff, 0, sizeof(g_buff));

        transfer_len = sl_Recv(g_SockID, &g_buff[0], MAX_BUFF_SIZE, 0);
        debug_printf( "transfer_len = %lu\n\r", transfer_len);

        if(transfer_len <= 0)
            ASSERT_ON_ERROR(TCP_RECV_ERROR);

        pBuff = g_buff;
    }

    return SUCCESS;
}

/*!
    \brief This function configure the SimpleLink device in its default state. It:
           - Sets the mode to STATION
           - Configures connection policy to Auto and AutoSmartConfig
           - Deletes all the stored profiles
           - Enables DHCP
           - Disables Scan policy
           - Sets Tx power to maximum
           - Sets power policy to normal
           - Unregisters mDNS services
           - Remove all filters

    \param[in]      none

    \return         On success, zero is returned. On error, negative is returned
*/
static _i32 configureSimpleLinkToDefaultState()
{
    SlVersionFull   ver = {{0}};
    _WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {{0}};

    _u8           val = 1;
    _u8           configOpt = 0;
    _u8           configLen = 0;
    _u8           power = 0;

    _i32          retVal = -1;
    _i32          mode = -1;

    debug_printf("sl_Start entering...\n\r");
    mode = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(mode);
    debug_printf("sl_Start ok\n\r");

    /* If the device is not in station-mode, try configuring it in station-mode */
    if (ROLE_STA != mode)
    {
        if (ROLE_AP == mode)
        {
            /* If the device is in AP mode, we need to wait for this event before doing anything */
            while(!IS_IP_ACQUIRED(g_Status)) { _SlNonOsMainLoopTask(); }
        }

        /* Switch to STA role and restart */
        retVal = sl_WlanSetMode(ROLE_STA);
        ASSERT_ON_ERROR(retVal);

        retVal = sl_Stop(SL_STOP_TIMEOUT);
        ASSERT_ON_ERROR(retVal);

        retVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(retVal);

        /* Check if the device is in station again */
        if (ROLE_STA != retVal)
        {
            /* We don't want to proceed if the device is not coming up in station-mode */
            ASSERT_ON_ERROR(DEVICE_NOT_IN_STATION_MODE);
        }
    }
    debug_printf("role set ok\n\r");

    /* Get the device's version-information */
    configOpt = SL_DEVICE_GENERAL_VERSION;
    configLen = sizeof(ver);
    retVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &configOpt, &configLen, (_u8 *)(&ver));
    ASSERT_ON_ERROR(retVal);
    debug_printf("dev version ok\n\r");

    /* Set connection policy to Auto + SmartConfig (Device's default connection policy) */
    retVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    debug_printf("wlan policy retVal = %d\n\r", retVal);
    ASSERT_ON_ERROR(retVal);
    debug_printf("wlan policy ok\n\r");

    /* Remove all profiles */
    retVal = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(retVal);
    debug_printf("wlan profile ok\n\r");

    /*
     * Device in station-mode. Disconnect previous connection if any
     * The function returns 0 if 'Disconnected done', negative number if already disconnected
     * Wait for 'disconnection' event if 0 is returned, Ignore other return-codes
     */
    retVal = sl_WlanDisconnect();
    if(0 == retVal)
    {
        /* Wait */
        while(IS_CONNECTED(g_Status)) { _SlNonOsMainLoopTask(); }
    }
    debug_printf("wlan Disconnected ok\n\r");

    /* Enable DHCP client*/
    retVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&val);
    ASSERT_ON_ERROR(retVal);
    debug_printf("DHCP enabled\n\r");

    /* Disable scan */
    configOpt = SL_SCAN_POLICY(0);
    retVal = sl_WlanPolicySet(SL_POLICY_SCAN , configOpt, NULL, 0);
    ASSERT_ON_ERROR(retVal);
    debug_printf("sl_WlanPolicySet SL_POLICY_SCAN ok\n\r");

    /* Set Tx power level for station mode
       Number between 0-15, as dB offset from maximum power - 0 will set maximum power */
    power = 0;
    retVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (_u8 *)&power);
    ASSERT_ON_ERROR(retVal);
    debug_printf("sl_WlanSet ok\n\r");

    /* Set PM policy to normal */
    retVal = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    ASSERT_ON_ERROR(retVal);
    debug_printf("sl_WlanPolicySet SL_POLICY_PM ok\n\r");

    /* Unregister mDNS services */
    retVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(retVal);
    debug_printf("sl_NetAppMDNSUnRegisterService ok\n\r");

    /* Remove  all 64 filters (8*8) */
    pal_Memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
    retVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask,
                       sizeof(_WlanRxFilterOperationCommandBuff_t));
    ASSERT_ON_ERROR(retVal);
    debug_printf("sl_WlanRxFilterSet ok\n\r");

    retVal = sl_Stop(SL_STOP_TIMEOUT);
    ASSERT_ON_ERROR(retVal);
    debug_printf("sl_Stop ok\n\r");

    retVal = initializeAppVariables();
    ASSERT_ON_ERROR(retVal);
    debug_printf("initializeAppVariables ok\n\r");

    return retVal; /* Success */
}

/*!
    \brief Connecting to a WLAN Access point

    This function connects to the required AP (FD_SSID_NAME).
    The function will return once we are connected and have acquired IP address

    \param[in]  None

    \return     0 on success, negative error-code on error

    \note

    \warning    If the WLAN connection fails or we don't acquire an IP address,
                We will be stuck in this function forever.
*/
static _i32 establishConnectionWithAP(uint8_t * ssid, int ssid_length, uint8_t * pass, int pass_length)
{
    SlSecParams_t secParams = {0};
    _i32 retVal = 0;
    _i32 timeout = 0;
    secParams.Key = (signed char *)pass;
    secParams.KeyLen = pass_length;
    secParams.Type = FD_SEC_TYPE;

    retVal = sl_WlanConnect((signed char *)ssid, ssid_length, 0, &secParams, 0);
    debug_printf("sl_WlanConnect ok\n\r");
    ASSERT_ON_ERROR(retVal);

    /* Wait */
    while((!IS_CONNECTED(g_Status)) || (!IS_IP_ACQUIRED(g_Status))) {
        msleep(10);
        if(timeout++ > 500)
            return -1;
     _SlNonOsMainLoopTask(); 
    }
    debug_printf("Device connected!\n\r");
    return SUCCESS;
}

/*!
    \brief This function initializes the application variables

    \param[in]  None

    \return     0 on success, negative error-code on error
*/
static _i32 initializeAppVariables()
{
    g_Status = 0;
    g_SockID = 0;
    g_DestinationIP = 0;
    g_BytesReceived = 0;
    pal_Memset(g_buff, 0, sizeof(g_buff));

    return SUCCESS;
}
