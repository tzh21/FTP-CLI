import socket
 
size = 8192

try:
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  
  for i in range(1, 51):
    msg = f"{i}"
    sock.sendto(msg.encode('utf-8'), ('localhost', 9876))
    print(sock.recv(size).decode('utf-8'))

  sock.close()

except Exception as e:
  print(f"Error: {e}")