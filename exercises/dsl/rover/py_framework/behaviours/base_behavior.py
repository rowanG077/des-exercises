"""
The behaviour module contains all behaviours that are used.
"""

from threading import Lock
from ev3dev2.sensor.lego import ColorSensor, UltrasonicSensor
from ev3dev2.motor import SpeedPercent
from ev3dev2.led import Leds

class BaseBehaviours:
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
