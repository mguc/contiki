# JN516x Remote Flashing for Raspberry Pi

This extension allows remote flashing of a connected JN516x module on a Raspberry Pi.

## Installation

Install all necessary python packages on the RPi, normally this is already done.
Disable the terminal/console on the RPi serial interface (See "Connection to a microcontroller or other peripheral" guide [here](http://elinux.org/RPi_Serial_Connection).)
To install simply execute "./init_pi.sh rpi_ip_address"

WARNING: If you have authorized keys under "~/.ssh/authorized_keys" and don't want them to be overwritten you should add the id_pi.pub key and the "jenprog" folder in your home folder manually.

## Usage

./remote_flash.sh path/to/binary rpi_ip_address
