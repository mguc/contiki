#include "simplelink.h"
#include "sl_common.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "log_helper.h"
#undef uint32_t
#define __cfunc__ (char*)__func__

void ping_result(SlPingReport_t* pReport)
{
    unsigned long loss = ((pReport->PacketsSent - pReport->PacketsReceived) / pReport->PacketsSent) * 100;
    log_msg(LOG_INFO, __cfunc__, "************************PING Test finished******************");
    log_msg(LOG_INFO, __cfunc__, "%lu packets transmitted, %lu received, %u%% packet loss, time %lums", pReport->PacketsSent, pReport->PacketsReceived, loss, pReport->TestTime);
    log_msg(LOG_INFO, __cfunc__, "rtt min/avg/max/mdev = %u/%u/%u/mdev ms", pReport->MinRoundTime, pReport->AvgRoundTime, pReport->MaxRoundTime);
    log_msg(LOG_INFO, __cfunc__, "************************************************************");
}

void get_mac_address(void)
{
    unsigned char macAddressVal[SL_MAC_ADDR_LEN];
    unsigned char macAddressLen = SL_MAC_ADDR_LEN;
    sl_NetCfgGet(SL_MAC_ADDRESS_GET,NULL,&macAddressLen,(unsigned char *)macAddressVal);

    log_msg(LOG_INFO, __cfunc__, "MAC: %02x:%02x:%02x:%02x:%02x:%02x", macAddressVal[0], macAddressVal[1], macAddressVal[2], macAddressVal[3], macAddressVal[4], macAddressVal[5]);
}

void get_ip_config(void)
{
    unsigned char len = sizeof(SlNetCfgIpV4Args_t);
    unsigned char dhcpIsOn = 0;
    SlNetCfgIpV4Args_t ipV4 = {0};
    
    sl_NetCfgGet(SL_IPV4_STA_P2P_CL_GET_INFO,&dhcpIsOn,&len,(unsigned char *)&ipV4);
                                 
    log_msg(LOG_INFO, __cfunc__, "DHCP is %s IP %d.%d.%d.%d MASK %d.%d.%d.%d GW %d.%d.%d.%d DNS %d.%d.%d.%d",
        (dhcpIsOn > 0) ? "ON" : "OFF",
        SL_IPV4_BYTE(ipV4.ipV4,3),SL_IPV4_BYTE(ipV4.ipV4,2),SL_IPV4_BYTE(ipV4.ipV4,1),SL_IPV4_BYTE(ipV4.ipV4,0),
        SL_IPV4_BYTE(ipV4.ipV4Mask,3),SL_IPV4_BYTE(ipV4.ipV4Mask,2),SL_IPV4_BYTE(ipV4.ipV4Mask,1),SL_IPV4_BYTE(ipV4.ipV4Mask,0),
        SL_IPV4_BYTE(ipV4.ipV4Gateway,3),SL_IPV4_BYTE(ipV4.ipV4Gateway,2),SL_IPV4_BYTE(ipV4.ipV4Gateway,1),SL_IPV4_BYTE(ipV4.ipV4Gateway,0),
        SL_IPV4_BYTE(ipV4.ipV4DnsServer,3),SL_IPV4_BYTE(ipV4.ipV4DnsServer,2),SL_IPV4_BYTE(ipV4.ipV4DnsServer,1),SL_IPV4_BYTE(ipV4.ipV4DnsServer,0));
}

int ping(const char* ip_address, unsigned int num_of_packets)
{
    int res;
    uint32_t a1, a2, a3, a4;
    SlPingReport_t report;
    SlPingStartCommand_t pingCommand;

    if(!num_of_packets)
        return 0;

    sscanf(ip_address, "%u.%u.%u.%u", &a1, &a2, &a3, &a4);
    log_msg(LOG_INFO, __cfunc__, "Starting ping for: %u.%u.%u.%u", a1, a2, a3, a4);

    pingCommand.Ip = SL_IPV4_VAL(a1,a2,a3,a4);
    pingCommand.PingSize = 64;                    // size of ping, in bytes 
    pingCommand.PingIntervalTime = 100;           // delay between pings, in milliseconds
    pingCommand.PingRequestTimeout = 1000;        // timeout for every ping in milliseconds
    pingCommand.TotalNumberOfAttempts = num_of_packets;
    pingCommand.Flags = 0;
    res = sl_NetAppPingStart( &pingCommand, SL_AF_INET, &report, NULL ) ;
    if(res < 0)
        return res;

    unsigned long loss = ((float)(report.PacketsSent - report.PacketsReceived) / (float)report.PacketsSent) * 100;
    log_msg(LOG_INFO, __cfunc__, "--- %u.%u.%u.%u ping statistics ---", a1, a2, a3, a4);
    log_msg(LOG_INFO, __cfunc__, "%lu packets transmitted, %lu received, %u%% packet loss, time %lums", report.PacketsSent, report.PacketsReceived, loss, report.TestTime);
    log_msg(LOG_INFO, __cfunc__, "rtt min/avg/max = %u/%u/%u ms(?)", report.MinRoundTime, report.AvgRoundTime, report.MaxRoundTime);

    return 0;
}