package ev3.behaviours.generator

import org.eclipse.xtext.generator.IFileSystemAccess2

class PythonFrameworkGenerator {
	def static generate(IFileSystemAccess2 fsa, String path)
	{
		fsa.generateFile(path + "slave.py", generateSlave());
		fsa.generateFile(path + "robot.py", generateRobot());
		fsa.generateFile(path + "base_behaviour.py", generateBaseBehaviour());
	}

	def static generateSlave()'''
	#!/usr/bin/env python3

	import json

	from ev3dev2.motor import OUTPUT_A, OUTPUT_D
	from ev3dev2._platform.ev3 import INPUT_3, INPUT_2, INPUT_4, INPUT_1
	from ev3dev2.sensor.lego import TouchSensor, UltrasonicSensor

	from robot import BluetoothConnection, RemoteSensor, MessageType


	def main():
	    ts_back = TouchSensor(INPUT_1)
	    ts_left = TouchSensor(INPUT_2)
	    ts_right = TouchSensor(INPUT_3)
	    us_front = UltrasonicSensor(INPUT_4)
	    us_front.mode = UltrasonicSensor.MODE_US_DIST_CM

	    conn = BluetoothConnection()
	    conn.connect("00:17:E9:B4:CE:E6", 44444)

	    while True:
	        sensordata = {
	            RemoteSensor.TS_BACK: ts_back.is_pressed,
	            RemoteSensor.TS_LEFT: ts_left.is_pressed,
	            RemoteSensor.TS_RIGHT: ts_right.is_pressed,
	            RemoteSensor.US_FRONT: us_front.value()
	        }

	        json_str = json.dumps({"type": MessageType.SENSORDATA, "data": sensordata})
	        conn.send(json_str)

	    conn.disconnect()
	    print("Slave stopped")

	if __name__ == '__main__':
	    main()
	'''

	def static generateBaseBehaviour()'''
	"""
	The behaviour module contains all behaviours that are used.
	"""

	from threading import Lock
	from ev3dev2.sensor.lego import ColorSensor, UltrasonicSensor
	from ev3dev2.motor import SpeedPercent
	from ev3dev2.led import Leds

	from robot import RemoteSensor

	class BaseBehaviour:
	    """
	    Behaviour is the base class for behaviours. Each subclass must override
	    action_gen and take_control method.
	    """

	    def __init__(self, robot, priority):
	        self.behaviour_priority = priority
	        self.robot = robot
	        self.started = False
	        self.suppress()

	    def priority(self):
	        """
	        Get the priority of the task. Higher means higher priority.
	        """

	        return self.behaviour_priority

	    def action_gen(self):
	        """
	        A subclass must override this in order to generate actions.
	        """

	        raise NotImplementedError()

	    def suppress(self):
	        """
	        Suppress current behaviour.
	        """

	        self.started = False
	        self.gen = self.action_gen()

	    def action(self):
	        """
	        Start executing a behaviour's action.
	        """

	        self.started = True
	        try:
	            next(self.gen)
	        except StopIteration:
	            self.started = False

	    def take_control(self):
	        """
	        A subclass must override this in order to obtain control on a specified
	        condition.
	        """

	        return self.started
	'''

	def static generateRobot()'''
	"""
	The robot module contains classes to construct a robot.
	"""

	from threading import Thread, Lock
	import json
	from time import sleep
	from bluetooth import BluetoothSocket, RFCOMM


	class MessageType:
	    """
	    MessageType contains all message types that bricks can send.
	    """

	    SENSORDATA = 1


	class RemoteSensor:
	    """
	    """

	    TS_BACK = "ts_back"
	    TS_LEFT = "ts_left"
	    TS_RIGHT = "ts_right"
	    US_FRONT = "us_front"


	class Robot:
	    """
	    The robot class represents a single robot.
	    """

	    def __init__(self, mac_server):
	        self.behaviors = []
	        self.mac_server = mac_server
	        self.conn = None
	        self.sensordata = {
	            RemoteSensor.TS_BACK: False,
	            RemoteSensor.TS_LEFT: False,
	            RemoteSensor.TS_RIGHT: False,
	            RemoteSensor.US_FRONT: 10000
	        }
	        self.sensordata_lock = Lock()

	    def __connect(self):
	        self.conn = BluetoothConnection()
	        port = 44444
	        self.conn.host(self.mac_server, port)
	        self.conn.listen(self.__on_message)

	    def __disconnect(self):
	        self.conn.disconnect()

	    def __set_sensordata(self, data):
	        with self.sensordata_lock:
	            for k in self.sensordata:
	                if k in data:
	                    self.sensordata[k] = data[k]

	    def __on_message(self, msg):
	        parsed = json.loads(msg)
	        msg_type = parsed["type"]

	        if msg_type == MessageType.SENSORDATA:
	            self.__set_sensordata(parsed["data"])

	    def get_sensordata(self, sensor):
	        """
	        Get sensor data gathered by different parts of the robot.
	        """
	        with self.sensordata_lock:
	            return self.sensordata[sensor]

	    def add_behaviour(self, behavior):
	        """
	        Add a behaviour to the robot.
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

	        encoded = data.encode()
	        if b'\0' in encoded:
	            raise ValueError("Data to send may not contain null characters.")

	        self.sock.sendall(encoded + b'\0')
	        self.sock.flush()

	    def __listen(self, callback):
	        buf = b''
	        while True:
	            data = self.sock.recv(4096)
	            if not data:
	                break

	            # XXX: encode() is required since the simulator's BluetoothSocket
	            # returns a decoded string rather than the byte object required by
	            # the standard.
	            buf += data.encode()

	            while b'\0' in buf:
	                s = buf.split(b'\0', 1)
	                callback(s[0])
	                buf = s[1]

	    def listen(self, callback):
	        """
	        Start listening for incoming messages. Creates a new thread where
	        callback is called when a message is received.
	        """

	        self.listen_thread = Thread(target=self.__listen, args=(callback,), daemon=True)
	        self.listen_thread.start()
	'''
}
