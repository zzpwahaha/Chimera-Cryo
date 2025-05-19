import socket
class EthernetClient:
    """
    A simple TCP client that connects to the EthernetServer,
    sends messages, and optionally waits for a response.
    """
    def __init__(self, host='6.1.1.71', port=8080):
        self.server_addr = (host, port)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def connect(self):
        """
        Establish a connection to the server.
        """
        self.sock.connect(self.server_addr)
        print(f"Connected to server at {self.server_addr}")

    def send(self, message: str):
        """
        Send a string message to the server.
        """
        data = message.encode('utf-8')
        self.sock.sendall(data)

    def receive(self, bufsize=1024) -> str:
        """
        Receive a response from the server.
        """
        data = self.sock.recv(bufsize)
        return data.decode('utf-8')

    def close(self):
        """
        Close the client socket.
        """
        self.sock.close()
        print("Connection closed")

if __name__=="__main__":
    client = EthernetClient(host='6.1.1.71', port=8080)
    client.connect()
    # client.send("Test message")
    # client.send("Zernike 12 0")
    # client.send("Zernike 4 -4")
    # client.send("Zernike 24 0")
    client.send("Zernike_pure 24 0")
    recv = client.receive()
    print(recv)
    client.close()