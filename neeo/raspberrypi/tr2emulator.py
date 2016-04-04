#!/usr/bin/python
import time
import serial
from struct import pack, unpack

commands = {
    'version' : 0x02, #T_JN_VERSION
    'list' : 0x03, #T_GET_CP6_LIST
    'status' : 0x04, #T_STATUS
    # 'T_WIFI_CREDS' : 0x05, this command is not used anymore
    'info' : 0x06, #T_BRAIN_INFO
    # 'T_REST_REQUEST' : 0x07, # this command is not used anymore
    # 'T_FIRMWARE_INFO' : 0x08, # this command is not used anymore
    'ip' : 0x09, #T_GET_LOCAL_IP
    # 'T_UPDATE_PUSH' : 0x0A, # 'T_DEBUG' : 0x16, # this message type is only avaiable for the JN
    'pair' : 0x0B, #T_PAIR_WITH_CP6
    'get' : 0x11, #T_REST_GET
    'post' : 0x12, #T_REST_POST
    'put' : 0x13, #T_REST_PUT
    'delete' : 0x14, #T_REST_DELETE
    'action' : 0x15, #T_TRIGGER_ACTION
    # 'T_DEBUG' : 0x16, # this message type is only avaiable for the JN
    'heartbeat' : 0x17 #T_HEARTBEAT
    # 'T_ERROR_RESPONSE' : 0xEE # this message type is only avaiable for the JN
}
msg_hdr = {
    'start' : 0x55,
    'message_type' : 0,
    'message_id' : 0,
    'length' : 0,
    'reserved' : 0,
    'crc' : 0
}
msg_payload = {
    'data' : '',
    'crc' : 0
}

def crc8(data):
    crc = 0
    for i in data:
        crc ^= (ord(i) << 8)
        for j in range(0,8):
            if(crc & 0x8000):
                crc ^= 0x8380

            crc <<= 1
    return (crc >> 8);

# configure the serial connections (the parameters differs on the device you are connecting to)
ser = serial.Serial(
	port='/dev/ttyAMA0',
	baudrate=115200,
	parity=serial.PARITY_NONE,
	stopbits=serial.STOPBITS_ONE,
	bytesize=serial.EIGHTBITS
)

if ser.isOpen() :
    print "Serial is already open"
else :
    ser.open()

print 'Enter your commands below.\r\nInsert "exit" to leave the application.'

input=1
while 1 :
	# get keyboard input
    input = raw_input(">> ")
    if len(input) == 0 :
        continue
    input = input.split()
    if input[0] == 'help' :
        for i in commands :
            print i
    if input[0] == 'exit' :
        ser.close()
        exit()
    try:
        print commands[input[0]]
        msg_hdr['message_type'] = commands[input[0]]
        if len(input) > 1 :
            msg_hdr['length'] = len(input[1])
            msg_payload['data'] = input[1]
        else:
            msg_hdr['length'] = 0
            msg_payload['data'] = ''
            msg_payload['crc'] = 0

        msg = pack('<BBBHH', msg_hdr['start'], msg_hdr['message_type'], msg_hdr['message_id'], msg_hdr['length'], msg_hdr['reserved'])
        msg += pack('<B', crc8(msg))
        if msg_hdr['length'] > 0 :
            msg += pack('<%is'%msg_hdr['length'], msg_payload['data'])
            msg_payload['crc'] = crc8(msg_payload['data'])
        msg += pack('<B', msg_payload['crc'])
        # print unpack('<BBBHHB', msg)
        print msg.encode('hex')
        print "Sending message with length: ", len(msg)
        ser.write(msg)
        out = ''
        while len(out) != 8 :
            if ser.inWaiting() > 0 :
                out += ser.read(1)
            if len(out) == 1 :
                if out != 'U' :
                    out = ''
        if crc8(out[0:8]) != 0 :
            print "Header CRC check failed"
            continue
        rx_msg_len = unpack('<H', out[3:5])
        rx_msg_len = rx_msg_len[0] + 1
        print "Expected message length: ", rx_msg_len
        out = ''
        while len(out) != rx_msg_len :
            if ser.inWaiting() > 0 :
                out += ser.read(1)
        if crc8(out[0:rx_msg_len]) != 0 :
            print "Payload CRC check failed"
            continue
        if len(out) > 1 :
            print out[0:(rx_msg_len - 1)]
            print "Received message with length: ", len(out)

    except KeyError:
        print "Wrong command. Type 'help' to get a list of possible commands."
