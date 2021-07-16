import socket
import sys

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the port
server_address = ("10.10.0.2", 8080)
print('starting up on %s port %s' % server_address, file=sys.stderr)
sock.bind(server_address)

# Listen for incoming connections
sock.listen(1)

while True:
    # Wait for a connection
    print('waiting for a connection', file=sys.stderr)
    connection, client_address = sock.accept()

    try:
	    print('connection from', client_address, file=sys.stderr)

	    # Receive the data in small chunks and retransmit it
	    while True:
	        data = connection.recv(4)
	        print('received "%s"' % data, file=sys.stderr)
	        if data:
	            for byte in range(4):
					print('byte ', byte, ':', format(int(binascii.hexlify(byte_buf[ii*12 + jj*4 + byte]), 16), '08b'))
	        else:
	            print('no more data from', client_address, file=sys.stderr)
	            break
            
    finally:
        # Clean up the connection
        connection.close()