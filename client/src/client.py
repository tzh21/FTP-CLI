#! /usr/bin/python3
import subprocess
import random
import time
import filecmp
import struct
import os
import shutil
import string
from ftplib import FTP

ftp = FTP()
ip_server = "127.0.0.1"
# ip_server = "166.111.83.113"
port_server = 6789
dir = "/home/tzh/network/ftp/tcp"
# file_name = "nothin"
file_name = "test114.txt"
full_path = dir + "/" + file_name

print(ftp.connect(ip_server, port_server))

print(ftp.login(user='anonymous', passwd='anonymous@'))

# print(ftp.login(user='thss', passwd='thss2023'))

# print(ftp.sendcmd('SYST'))

print(ftp.sendcmd('TYPE I'))

ftp.set_pasv(True)

# ftp.pwd()
print(ftp.pwd())

print(ftp.rename("file.txt", "file114.txt"))
# print(ftp.mkd("test2"))

print(ftp.dir())

# print(ftp.cwd("test"))

# print(ftp.retrbinary("RETR %s" % file_name, open(file_name, 'wb').write))

# print(ftp.storbinary("STOR %s" % file_name, open(file_name, 'rb')))

print(ftp.quit())