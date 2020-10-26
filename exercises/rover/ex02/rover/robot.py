"""
The robot module contains classes to construct a robot.
"""

from threading import Thread
import json
from time import sleep
from bluetooth import BluetoothSocket, RFCOMM


class Robot:
    """
    The robot class represents a single robot.
    """

    def __init__(self, mac_server, is_server):
        self.behaviors = []
        self.mac_server = mac_server
        self.is_server = is_server
        self.data_receivers = {}
        self.conn = None

    def __connect(self):
        self.conn = BluetoothConnection()
        port = 44444
        if self.is_server:
            self.conn.host(self.mac_server, port)
        else:
            self.conn.connect(self.mac_server, port)

        self.conn.listen(self.__on_message)

    def __disconnect(self):
        self.conn.disconnect()

    def __on_message(self, msg):
        parsed = json.loads(msg)
        msg_type = parsed["type"]
        for receiver in self.data_receivers.get(msg_type, []):
            receiver(parsed["data"])

    def register_message_receiver(self, msg_type, receiver):
        """
        Register a receiver for incoming messages of the given type.
        """

        if msg_type not in self.data_receivers:
            self.data_receivers[msg_type] = []
        self.data_receivers[msg_type].append(receiver)

    def post_message(self, msg_type, msg):
        """
        Send a message to the other robot.
        """

        json_str = json.dumps({"type": msg_type, "data": msg})
        self.conn.send(json_str)

    def add_behavior(self, behavior):
        """
        Add a behavior to the robot.
        """

        self.behaviors.append(behavior)

    def run(self):
        """
        Start running behaviors until completion.
        """

        self.__connect()

        arbitrator = Arbitrator(self.behaviors)
        arbitrator.schedule()

        self.__disconnect()


class Arbitrator:
    """
    The arbitrator runs the scheduler for behaviors.
    """

    def __init__(self, behaviors):
        self.behaviors = sorted(behaviors, key=lambda x: x.priority(),
                                reverse=True)

    def needs_control(self):
        """
        Get the behavior with the highest priority that wants control.
        """

        for behavior in self.behaviors:
            if behavior.take_control():
                return behavior

        return None

    def schedule(self):
        """
        Run the behavior scheduler until no behavior wants control.
        """

        last_behavior = self.needs_control()
        prio_behavior = last_behavior
        while prio_behavior is not None:
            if last_behavior != prio_behavior:
                last_behavior.suppress()

            prio_behavior.action()
            last_behavior = prio_behavior
            prio_behavior = self.needs_control()
            # Small sleep to avoid 100% busy loops.
            sleep(0.001)


class BluetoothConnection:
    """
    BluetoothConnection contains functionality to create bluetooth connections.
    """

    def __init__(self):
        self.listen_thread = None
        self.sock = None

    def host(self, address, port):
        """
        Start hosting a server.
        """

        client_count = 1
        server_sock = BluetoothSocket(RFCOMM)
        server_sock.bind((address, port))
        server_sock.listen(client_count)
        print("Waiting for connections...")
        client_sock, client_address = server_sock.accept()
        print(f"Accepted connection from {client_address}")
        self.sock = client_sock

    def connect(self, address, port):
        """
        Connect to the given address on the given port.
        """

        sock = BluetoothSocket(RFCOMM)
        print("Connecting...")
        sock.connect((address, port))
        print(f"Connected to {address}")
        self.sock = sock

    def disconnect(self):
        """
        Disconnect the currently open connections.
        """

        self.sock.close()

    def send(self, data):
        """
        Send data to the connected device.
        """

        self.sock.sendall(data.encode())
        self.sock.flush()

    def __listen(self, callback):
        while True:
            data = self.sock.recv(4096)
            if not data:
                break

            callback(data)

    def listen(self, callback):
        """
        Start listening for incoming messages. Creates a new thread where
        callback is called when a message is received.
        """

        self.listen_thread = Thread(target=self.__listen, args=(callback,), daemon=True)
        self.listen_thread.start()
