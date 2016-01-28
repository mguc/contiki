#!/usr/bin/python
import time
import serial
from array import *
from struct import pack
# struct hdr_t {
#         uint8_t start;          // always 0x55
#         uint8_t message_type;   // for now always 1
#         uint8_t message_id;     //identifier of the message
#         uint16_t len;           // payload len, can be 0, max 8kB, if exists includes the crc8 at the end
#         uint16_t reserved;      // unused now
#         uint8_t crc;            // crc of a header
# }  __attribute__((packed)) hdr_t;
# class Header:
#     start = array('B', '\x55')
#
# class Message:

def crc8(data):
    crc = 0
    for i in data:
        print i
        crc ^= (ord(i) << 8)
        print hex(crc);
        for j in range(0,8):
            if(crc & 0x8000):
                crc ^= 0x8380

            crc <<= 1
            print hex(crc)
    return hex(crc >> 8);


# configure the serial connections (the parameters differs on the device you are connecting to)
# ser = serial.Serial(
# 	port='/dev/ttyAMA0',
# 	baudrate=115200,
# 	parity=serial.PARITY_ODD,
# 	stopbits=serial.STOPBITS_NONE,
# 	bytesize=serial.EIGHTBITS
# )

# ser.open()
# ser.isOpen()
#
# print 'Enter your commands below.\r\nInsert "exit" to leave the application.'
#
# input=1
# while 1 :
# 	# get keyboard input
# 	input = raw_input(">> ")
#         # Python 3 users
#         # input = input(">> ")
# 	if input == 'exit':
# 		ser.close()
# 		exit()
# 	else:
# 		# send the character to the device
# 		# (note that I happend a \r\n carriage return and line feed to the characters - this is requested by my device)
# 		ser.write(input)
# 		out = ''
# 		# let's wait one second before reading output (let's give device time to answer)
# 		time.sleep(1)
# 		while ser.inWaiting() > 0:
# 			out += ser.read(1)
#
# 		if out != '':
# 			print ">>" + out
