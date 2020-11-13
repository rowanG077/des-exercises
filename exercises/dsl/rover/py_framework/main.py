#!/usr/bin/env python3

import argparse

from ev3dev2.motor import MoveTank, OUTPUT_A, OUTPUT_D
from ev3dev2._platform.ev3 import INPUT_3, INPUT_2, INPUT_4, INPUT_1
from ev3dev2.sensor.lego import ColorSensor, TouchSensor, UltrasonicSensor

from robot import Robot
import behaviours.behaviours as b


def main():
    parser = argparse.ArgumentParser(description="ev3dev program to search for red, yellow, and blue spots")
    parser.add_argument("address", help="MAC address of the server robot")
    parser.add_argument("-s", "--server", help="Set the current robot as the server",
                        action="store_true")
    args = parser.parse_args()

    us = UltrasonicSensor(INPUT_3)
    us.mode = UltrasonicSensor.MODE_US_DIST_CM
    cs_mid = ColorSensor(INPUT_2)
    move_tank = MoveTank(OUTPUT_A, OUTPUT_D)
    ts_right = TouchSensor(INPUT_4)
    ts_left = TouchSensor(INPUT_1)

    robot = Robot(args.address, args.server)
    robot.add_behaviour(b.StayInBoundsBehaviour(robot, 90, cs_mid, move_tank))
    robot.add_behaviour(b.UltrasonicDistanceBehaviour(robot, 80, us, move_tank))
    robot.add_behaviour(b.AvoidTouchObjectBehaviour(robot, 70, ts_left, ts_right, move_tank))
    robot.add_behaviour(b.SearchBehaviour(robot, 10, cs_mid, move_tank))
    robot.run()

    print("Robot stopped")

if __name__ == '__main__':
    main()
