require 'socket'
port = 3000
sock = UDPSocket.new
data = 'Hello Server'
sock.send(data, 0, '127.0.0.1', port)
resp = sock.recv(1024)
p resp
sock.close
