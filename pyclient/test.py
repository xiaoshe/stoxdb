import socket
import struct
import time

host = "127.0.0.1"
port = 30634

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.settimeout(5)
sock.connect((host, port))


def build_packet(buff):
    magic = 41401891
    opt = 1
    size = len(buff)

    data = struct.pack("iii", magic, size, opt)
    return data + buff.encode('utf-8')


buff = build_packet("hello")


time.sleep(5)

try:
    sock.sendall(buff)
    resp = sock.recv(1024)
    a, b, c = struct.unpack("iii", resp[0:12])
    msg = resp[12:].decode()
    print(a, b, c, msg)
except Exception as e:
    print(e)
