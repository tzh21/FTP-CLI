import socket

size = 8192

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('', 9876))

seq_num = 0

try:
  while True:
    data, address = sock.recvfrom(size)
    seq_num += 1
    response = f"{seq_num} {data.decode('utf-8')}"
    sock.sendto(response.encode('utf-8'), address)
finally:
  sock.close()