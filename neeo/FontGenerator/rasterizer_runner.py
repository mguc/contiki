#!/usr/bin/python2

__author__ = 'zborowski'

import subprocess
import logging
import logging.handlers

from bs4 import BeautifulSoup


class RasterizerRunner:
    DEFAULT_HOME_PATH = "."
    DEFAULT_CONFIG_FILE_PATH = "../romfs/gui.xml"
    DEFAULT_RASTERIZER_TOOL_PATH = DEFAULT_HOME_PATH + "/ftgen"
    DEFAULT_BPP = "8"
    DEFAULT_INPUT_PATH = DEFAULT_HOME_PATH + "/Fonts"
    DEFAULT_OUTPUT_PATH = "../romfs/fonts"
    DEFAULT_TEMP_FILE_NAME = DEFAULT_HOME_PATH + "/chars.tmp"
    DEFAULT_LOG_FILE = DEFAULT_HOME_PATH + "/RasterizerRunner.log"

    DEFAULT_CHAR_SET = {
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e    }

    def __init__(self):
        self.__config_file_path = self.DEFAULT_CONFIG_FILE_PATH
        self.__rasterizer_file_path = self.DEFAULT_RASTERIZER_TOOL_PATH
        self.__input_path = self.DEFAULT_INPUT_PATH
        self.__output_path = self.DEFAULT_OUTPUT_PATH
        self.__temp_file_name = self.DEFAULT_TEMP_FILE_NAME
        self.__logger = logging.getLogger("RasterizerRunner")

    def __build_tmp_file(self, data, cmd):
        fd = open(self.__temp_file_name, "w+")
        data_str = cmd + "=" + ", ".join(map(lambda v: "0x%04X" % v, list(data)))
        fd.write(data_str)
        fd.close()

    def __run_rasterizer(self, name, props):
        self.__logger.info("Processing font from file: %s" % self.__input_path + "/" + props["file"])
        try:
            args = [self.__rasterizer_file_path, self.__input_path + "/" + props["file"],
                    props["size"], self.DEFAULT_BPP, "--binary-out",
                    self.__output_path + "/" + name + ".font"]

            if "char_list" in props.keys():
                self.__build_tmp_file(props["char_list"], "SET_CHARS")
                args.append(self.__temp_file_name)
                args.append("--no-autokern")

            if "append_char_list" in props.keys():
                self.__build_tmp_file(props["append_char_list"], "APPEND_CHARS")
                args.append(self.__temp_file_name)

            if subprocess.call(args) != 0:
                self.__logger.error(
                    "Rasterizer process returns error for font: %s, size: %s" % (props["file"], props["size"]))

        except KeyError, ke:
            self.__logger.error("Arguments error: %s" % ke.message)

        except ValueError, ve:
            self.__logger.error("Conversion error: %s" % ve.message)

        except IOError, ie:
            self.__logger.error("File error: %s" % ie.strerror)

        except TypeError, te:
            self.__logger.error("Type error: %s" % te.message)

        except OSError, oe:
            self.__logger.error("Rasterizer error: %s" % oe.strerror)

        except Exception, ee:
            self.__logger.error("Unrecognized exception: %s" % ee.__class__)

    def __parse_styles(self, soup, style_tag):
        self.__logger.debug("Parsing styles section for %s" % style_tag)
        styles = {}

        style_list = soup.findAll(style_tag)
        for style_item in style_list:
            name = style_item.get("name")
            file_name = style_item.get("file")
            size = style_item.get("size")

            self.__logger.debug("Found style: %s, file: %s, size: %s" % (name, file_name, size))
            styles[name] = {"file": file_name, "size": size}

        return styles

    def __parse_screen_config(self, soup):
        self.__logger.debug("Parsing screen config")

        font_items = {}
        ico_items = {}
        output = {"FONTS": font_items, "ICONS": ico_items}

        # Find icons
        elements_with_icons = soup.findAll(attrs={'icoChar': True})

        for e in elements_with_icons:
            style_name = e.get('icoFont', '')
            ico_items.setdefault(style_name, set())

            try:
                value_str = e.get('icoChar', '')
                if value_str[0] == "#":
                    value = int(value_str[1:], 16)
                else:
                    value = int(value_str)

                ico_items[style_name].add(value)

            except ValueError:
                self.__logger.error("Error in converting icoChar: %s" % value_str)

        # Find text strings
        elements_with_text = soup.findAll(attrs={'text': True})

        for e in elements_with_text:
            style_name = e.get('font', '')
            font_items.setdefault(style_name, set())

            for letter in e.get('text', ''):
                code = ord(letter)
                if code not in self.DEFAULT_CHAR_SET:
                    font_items[style_name].add(code)

        elements_with_description = soup.findAll(attrs={'descriptionText': True})

        for e in elements_with_description:
            style_name = e.get('descriptionFont', '')
            font_items.setdefault(style_name, set())

            for letter in e.get('descriptionText', ''):
                code = ord(letter)
                if code not in self.DEFAULT_CHAR_SET:
                    font_items[style_name].add(code)

        return output

    def parse_config(self):
        self.__logger.info("Parsing config file: %s" % self.__config_file_path)
        output_config = {}

        soup = BeautifulSoup(open(self.__config_file_path), 'xml')

        # Parse styles section
        font_styles = self.__parse_styles(soup, "font-style")
        #ico_styles = self.__parse_styles(soup, "ico-style")

        # Parse general part of config to collect all styles names,
        # and list of characters for ico styles
        screen_config = self.__parse_screen_config(soup)

        found_font_styles = screen_config["FONTS"]
        found_ico_styles = screen_config["ICONS"]

        output_config["FONT_STYLES"] = font_styles
        #output_config["ICO_STYLES"] = ico_styles

        # Update font styles by char list
        for style_name, style_config in font_styles.items():
            if style_name in screen_config["FONTS"].keys():
                if len(found_font_styles[style_name]) > 0:
                    output_config["FONT_STYLES"][style_name]["append_char_list"] = found_font_styles[style_name]

        # Update ico styles by char list
        for style_name, style_config in font_styles.items():
            if style_name in screen_config["ICONS"].keys():
                if len(found_ico_styles[style_name]) > 0:
                    output_config["FONT_STYLES"][style_name]["char_list"] = found_ico_styles[style_name]

        # Handle undefined styles
        for style_name in found_font_styles.keys():
            if style_name not in font_styles.keys():
                self.__logger.error("Missing definition for font style: %s" % style_name)

        #for style_name in found_ico_styles.keys():
        #    if style_name not in ico_styles.keys():
        #        self.__logger.error("Missing definition for ico style: %s" % style_name)

        return output_config

    def process_font(self, in_config):
        self.__logger.info("Processing fonts")

        for style_name, props in in_config["FONT_STYLES"].items():
            self.__logger.debug("Style name: %s props: %s" % (style_name, props))
            self.__run_rasterizer(style_name, props)

        #for style_name, props in in_config["ICO_STYLES"].items():
        #    self.__logger.debug("Style name: %s props: %s" % (style_name, props))
        #    self.__run_rasterizer(style_name, props)

if __name__ == "__main__":

    # Get logging utility
    logger = logging.getLogger()
    logger.setLevel(logging.INFO)
    # logger.setLevel(logging.DEBUG)

    # Set console log handler
    handler = logging.StreamHandler()
    logger.addHandler(handler)

    # Set log file handler
    handler = logging.handlers.RotatingFileHandler(RasterizerRunner.DEFAULT_LOG_FILE, maxBytes=48000, backupCount=1)
    formatter = logging.Formatter("%(asctime)s %(levelname)-8s %(message)s")
    handler.setFormatter(formatter)
    logger.addHandler(handler)

    runner = RasterizerRunner()
    config = runner.parse_config()
    runner.process_font(config)
