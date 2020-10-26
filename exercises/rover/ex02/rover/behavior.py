"""
The behavior module contains all behaviors that are used.
"""

from threading import Lock
from ev3dev2.sensor.lego import ColorSensor, UltrasonicSensor
from ev3dev2.motor import SpeedPercent
from ev3dev2.led import Leds


class MessageType:
    """
    MessageType contains all message types that behaviors can send.
    """

    TARGETSFOUND = 1

class Behavior:
    """
    Behavior is the base class for behaviors. Each subclass must override
    action_gen and take_control method.
    """

    def __init__(self, robot, priority):
        self.behavior_priority = priority
        self.robot = robot
        self.started = False
        self.suppress()

    def priority(self):
        """
        Get the priority of the task. Higher means higher priority.
        """

        return self.behavior_priority

    def action_gen(self):
        """
        A subclass must override this in order to generate actions.
        """

        raise NotImplementedError()

    def suppress(self):
        """
        Suppress current behavior.
        """

        self.started = False
        self.gen = self.action_gen()

    def action(self):
        """
        Start executing a behavior's action.
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


class StayInBoundsBehavior(Behavior):
    """
    This behavior tries to stay inside the area's boundary.
    """

    def __init__(self, robot, priority, cs, tank):
        super().__init__(robot, priority)
        self.cs = cs
        self.tank = tank

    def action_gen(self):
        self.tank.stop()
        yield

        self.tank.on_for_rotations(SpeedPercent(-35), SpeedPercent(-35), 1, block=False)
        while self.tank.is_running:
            yield

        self.tank.on_for_degrees(SpeedPercent(40), SpeedPercent(1), 250, block=False)
        while self.tank.is_running:
            yield

    def take_control(self):
        return super().take_control() or self.cs.color == ColorSensor.COLOR_BLACK


class SearchBehavior(Behavior):
    """
    This behavior tries to search for the three coloured spots.
    """

    def __init__(self, robot, priority, cs, tank):
        super().__init__(robot, priority)
        self.cs = cs
        self.tank = tank
        self.targets_lock = Lock()
        self.leds = Leds()
        self.targets = {
            ColorSensor.COLOR_YELLOW: False,
            ColorSensor.COLOR_BLUE: False,
            ColorSensor.COLOR_RED: False
        }
        self.led_brightness = 0.0

        self.robot.register_message_receiver(MessageType.TARGETSFOUND, self.on_message)

    def trigger_led(self):
        """
        Increment the LED brightness when a target has been found.
        """

        self.led_brightness += 0.33
        self.leds.set_color('LEFT', 'GREEN', pct=self.led_brightness)

    def on_message(self, message):
        """
        Merge the other robot's state with our state.
        """

        with self.targets_lock:
            for k in self.targets:
                res = message.get(str(k), False)
                if res:
                    self.targets[k] = True
                    self.trigger_led()

    def found(self):
        """
        Check if all targets have been found.
        """

        with self.targets_lock:
            return all(self.targets.values())

    def action_gen(self):
        self.tank.on(SpeedPercent(30), SpeedPercent(30))

        while not self.found():
            current_color = self.cs.color
            color_str = self.cs.color_name
            updated = False
            with self.targets_lock:
                if current_color in self.targets and not self.targets[current_color]:
                    print("Found {}".format(color_str))
                    self.targets[current_color] = True
                    self.trigger_led()
                    updated = True
            yield
            if updated:
                self.robot.post_message(MessageType.TARGETSFOUND, self.targets)
                yield

        self.tank.stop()

    def take_control(self):
        return super().take_control() or not self.found()


class UltrasonicDistanceBehavior(Behavior):
    """
    This behavior tries to avoid objects that are within a certain range of
    the ultrasonic sensor.
    """

    def __init__(self, robot, priority, us, tank):
        super().__init__(robot, priority)
        self.us = us
        self.us.mode = UltrasonicSensor.MODE_US_DIST_CM
        self.tank = tank

    def action_gen(self):
        self.tank.stop()
        yield

        self.tank.on_for_rotations(SpeedPercent(-35), SpeedPercent(-35), 1, block=False)
        while self.tank.is_running:
            yield

        self.tank.on_for_degrees(SpeedPercent(40), SpeedPercent(1), 250, block=False)
        while self.tank.is_running:
            yield

    def take_control(self):
        distance = self.us.value()
        return super().take_control() or distance < 250


class AvoidTouchObjectBehavior(Behavior):
    """
    This behavior tries to avoid objects when the touch sensors detect
    something.
    """

    def __init__(self, robot, priority, tleft, tright, tank):
        super().__init__(robot, priority)
        self.tleft = tleft
        self.tright = tright
        self.tank = tank

    def action_gen(self):
        self.tank.stop()
        yield

        self.tank.on_for_rotations(SpeedPercent(-35), SpeedPercent(-35), 1, block=False)
        while self.tank.is_running:
            yield

        self.tank.on_for_degrees(SpeedPercent(40), SpeedPercent(1), 250, block=False)
        while self.tank.is_running:
            yield

    def take_control(self):
        touched = self.tleft.is_pressed or self.tright.is_pressed
        return super().take_control() or touched
