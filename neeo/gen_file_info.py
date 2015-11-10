#!/usr/bin/env python

import argparse
import zlib
import lxml.etree as et
import struct


def crc32_of_file(file):
    prev = 0
    for eachLine in open(file, "rb"):
        prev = zlib.crc32(eachLine, prev)
    return (prev & 0xffffffff)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generator of XML with file'
                                     'information')
    parser.add_argument('-d', '--description', action='store', nargs='?',
                        help="Optional description of file")
    parser.add_argument('-f', '--file', action='store', nargs='?',
                        required=True,
                        help="The file for which is created a description.")
    parser.add_argument('-l', '--location', action='store', nargs='?',
                        required=True,
                        help="Location of the file on server (eg. /firmware/")
    args = parser.parse_args()

    output_file = args.file[:-4] + ".xml"
    # print("Output file: {}".format(output_file))
    file_name = args.file[args.file.rfind("/")+1:]
    # print("File name: {}".format(file_name))
    file = open(args.file, "rb")
    val = file.read(4)
    version_string = struct.unpack('<i', val)[0]
    # print("Version: {}".format(version_string))
    if(args.location[-1] is not "/"):
        args.location = args.location + "/"
    # print("location {}".format(args.location))

    root = et.Element("firmware")
    et.SubElement(root, "name").text = args.description
    et.SubElement(root, "file").text = args.location + file_name
    et.SubElement(root, "version").text = "{}".format(version_string)
    et.SubElement(root, "crc32").text = "0x{:x}".format(crc32_of_file(args.file))
    tree = et.ElementTree(root)
    tree.write(output_file, pretty_print=True)
