import os
import sys

if len(sys.argv) < 4:
    sys.exit("Usage: File-name   Table-name   Output-File-Name")

f = open(str(sys.argv[1]), "r")
fo = open(str(sys.argv[3]), "w")
ln = 0
fo.write("const unsigned char %s[] = {\r\n" % sys.argv[2])
while 1 == 1:
    try:
        b = f.read(1)
        if b == '':
            break
        if ln != 0:
            fo.write(", ")
        else:
            fo.write(" ")
        if (ln % 16) == 0:
            fo.write("\r\n")
        fo.write("0x%02X" % ord(b))
        ln += 1
    except:
        break
fo.write("};")
