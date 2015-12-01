#!/usr/bin/env python
from coapthon.resources.resource import Resource
from coapthon.server.coap_protocol import CoAP
from netifaces import AF_INET
import lxml.etree as et
import netifaces as ni
import zlib
import struct

wlan_interface = 'eth2'


def crc32_of_file(f):
    prev = 0
    for eachLine in f:
        prev = zlib.crc32(eachLine, prev)
    return (prev & 0xffffffff)


class ConfigResource(Resource):
    def __init__(self, name="ConfigResource", coap_server=None):
        super(ConfigResource, self).__init__(name, coap_server, visible=True,
                                             observable=True,
                                             allow_children=True)

    def render_GET(self, request):
        xml = et.parse("/var/www/webapp/template/zero_conf.xml")
        try:
            brain_ip = ni.ifaddresses(wlan_interface)[AF_INET][0]['addr']
        except:
            brain_ip = "0.0.0.0"
        xml.find("brain_ip").text = brain_ip

        self.payload = et.tostring(xml, encoding='utf8')
        return self


class PowerSaveResource(Resource):
    def __init__(self, name="PowerSaveResource", coap_server=None):
        super(PowerSaveResource, self).__init__(name, coap_server,
                                                visible=True,
                                                observable=True,
                                                allow_children=True)

    def render_POST(self, request):
        print("Power save enabled!")
        self.payload = "ACK"
        return self


class FwInfoResource(Resource):
    def __init__(self, name="FwInfoResource", coap_server=None):
        super(FwInfoResource, self).__init__(name, coap_server, visible=True,
                                             observable=True,
                                             allow_children=True)

    def render_GET(self, request):
        print("Fw info uri: " + request.uri_path)
        fw_type = request.uri_path.split('/')[1]
        fw_file = {"neeo": "neeo.bin", "jn5168": "jn.bin"}
        root = et.Element("firmware")
        et.SubElement(root, "name").text = "Firmware file"
        try:
            f = open("/var/www/webapp/firmware/" + fw_file[fw_type], "rb")
            if fw_type == "neeo":
                f.seek(8)
            val = f.read(4)
            version_string = struct.unpack('<i', val)[0]
            # print("Version: {}".format(version_string))
            crc32 = crc32_of_file(f)
            f.close
            et.SubElement(root, "file").text = "/firmware/" + fw_file[fw_type]
        except IOError:
            crc32 = 0
            version_string = 0
            et.SubElement(root, "file").text = ""
        et.SubElement(root, "version").text = "{}".format(version_string)
        et.SubElement(root, "crc32").text = "0x{:x}".format(crc32)
        xml = et.ElementTree(root)
        self.payload = et.tostring(xml, encoding='utf8')
        return self


class TestResource(Resource):
    def __init__(self, name="TestResource", coap_server=None):
        super(TestResource, self).__init__(name, coap_server, visible=True,
                                           observable=True,
                                           allow_children=True)

    def render_GET(self, request):
        print("GET uri: " + request.uri_path)
        self.payload = "data response"
        return self

    def render_POST(self, request):
        print("POST uri: " + request.uri_path + " payload: " + request.payload)
        self.payload = "POST OK"
        return self

    def render_PUT(self, request):
        print("PUT uri: " + request.uri_path + " payload: " + request.payload)
        self.payload = "PUT OK"
        return self

    def render_DELETE(self, request):
        print("DELETE uri: " + request.uri_path)
        return True


class CoAPServer(CoAP):
    def __init__(self, host, port, multicast=False):
        CoAP.__init__(self, (host, port), multicast)
        self.add_resource('/config', Resource(name="Dummy", coap_server=None))
        self.add_resource('/config/zero_conf.xml', ConfigResource())
        self.add_resource('/resource', TestResource())
        self.add_resource('/power_save_enabled', PowerSaveResource())
        self.add_resource('/update', FwInfoResource())
        self.add_resource('/update/neeo', FwInfoResource())
        self.add_resource('/update/jn5168', FwInfoResource())
        print("CoAP Server start on " + host + ":" + str(port))
        print(self.root.dump())


if __name__ == "__main__":
    ip = "::"
    port = 3901

    server = CoAPServer(ip, port)
    try:
        server.listen(10)
    except KeyboardInterrupt:
        print("Server Shutdown")
        server.close()
        print("Exiting...")
