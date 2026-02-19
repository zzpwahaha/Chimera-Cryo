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
        if self.sock.fileno() == -1:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
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
    from time import sleep
    # client = EthernetClient(host='6.1.1.71', port=8080)
    client = EthernetClient(host='10.10.0.14', port=8080)
    client.connect()
    # client.send("Zernike 4 0.53")
    # client.send("Test message")
    # client.send("Zernike 12 0")
    # client.send("Zernike 4 -4")
    # client.send("Zernike 24 0")
    # client.send("Zernike 4 2.2")
    # client.send("Zernike_pure 24 0")

    # client.send("Phase-Pattern 1x1_bottle_beam")
    # client.send("Phase-Pattern 1x1_bottle_beam_fat")
    # client.send("Phase-Pattern 1x1_bottle_beam_interm")
    # client.send("Zernike 4 2.2")
    # client.send("Zernike 4 0")
    client.send("Zernike 4 3")


    # client.send("Phase-Pattern 5x20_20umx95um_trapBalanceCamera_cameraBalanced_balanced4")
    # # client.send("Phase-Pattern 1x7_latticeconstant16.5um_trapBalanceCamera_cameraBalanced_balanced2")
    # recv = client.receive()
    # print(recv)
    # client.close()

    # sleep(2)
    # client.connect()
    # client.send("Phase-Pattern 1x7_latticeconstant16.5um_trapBalanceCamera_cameraBalanced_balanced2")
    recv = client.receive()
    print(recv)
    client.close()

