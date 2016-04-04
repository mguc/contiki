__author__ = 'antmicro'

import subprocess
import os.path
import time


class PinController():

    # GPIO Linux path
    GPIO_PATH = "/sys/class/gpio/"

    # AWSom module PE2/CSI0-HSYNC
    # (4 * 32) + 2
    RESET_PIN = "2"

    # AWSom module PE3/CSI0-VSYNC
    # (4 * 32) + 3
    SPI_MOSI_PIN = "3"

    def __init__(self):
        pass

    @classmethod
    def configure(cls):
        # Make GPIO pins available for control and set as outputs.
        if not os.path.exists(cls.GPIO_PATH + "gpio" + cls.RESET_PIN):
            subprocess.call("echo " + cls.RESET_PIN + " > " + cls.GPIO_PATH + "export", shell=True)
            subprocess.call("echo out > " + cls.GPIO_PATH + "gpio" + cls.RESET_PIN + "/direction", shell=True)

        if not os.path.exists(cls.GPIO_PATH + "gpio" + cls.SPI_MOSI_PIN):
            subprocess.call("echo " + cls.SPI_MOSI_PIN + " > " + cls.GPIO_PATH + "export", shell=True)
            subprocess.call("echo out > " + cls.GPIO_PATH + "gpio" + cls.SPI_MOSI_PIN + "/direction", shell=True)

    @classmethod
    def enable_flashing_mode(cls):
        # Enter chip in the flashing mode.
        subprocess.call("echo 0 > " + cls.GPIO_PATH + "gpio" + cls.SPI_MOSI_PIN + "/value", shell=True)
        subprocess.call("echo 0 > " + cls.GPIO_PATH + "gpio" + cls.RESET_PIN + "/value", shell=True)
        time.sleep(0.005)
        subprocess.call("echo 1 > " + cls.GPIO_PATH + "gpio" + cls.RESET_PIN + "/value", shell=True)
        time.sleep(0.200)
        subprocess.call("echo 1 > " + cls.GPIO_PATH + "gpio" + cls.SPI_MOSI_PIN + "/value", shell=True)
        time.sleep(0.100)
