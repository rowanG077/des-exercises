#!/usr/bin/env python3

from ev3dev2.display import Display
from ev3dev2.motor import MoveTank, OUTPUT_A, OUTPUT_D, SpeedPercent
from ev3dev2._platform.ev3 import INPUT_3, INPUT_2, INPUT_4, INPUT_1
from ev3dev2.sensor.lego import ColorSensor, TouchSensor, UltrasonicSensor
from ev3dev2.sound import Sound

def drive():
    tank_drive.on(SpeedPercent(30), SpeedPercent(30))


def measure_distance():
    return us.value() / 10


my_display = Display()

us = UltrasonicSensor(INPUT_3)
us.mode = UltrasonicSensor.MODE_US_DIST_CM

ts_right = TouchSensor(INPUT_4)
ts_left = TouchSensor(INPUT_1)

tank_drive = MoveTank(OUTPUT_A, OUTPUT_D)
cs = ColorSensor(INPUT_2)

s = Sound()

my_display.clear()

drive()
while True:
    if cs.color == ColorSensor.COLOR_BLACK:
        tank_drive.stop()
        s.beep()
        tank_drive.on_for_rotations(SpeedPercent(-35), SpeedPercent(-35), 1)
        tank_drive.on_for_degrees(SpeedPercent(40), SpeedPercent(0), 250)
        drive()
    elif measure_distance() < 25:
        tank_drive.stop()
        s.beep()
        tank_drive.on_for_rotations(SpeedPercent(-35), SpeedPercent(-35), 1)
        tank_drive.on_for_degrees(SpeedPercent(40), SpeedPercent(0), 300)
        drive()
    elif ts_right.is_pressed:
        tank_drive.stop()
        s.beep()
        tank_drive.on_for_rotations(SpeedPercent(-35), SpeedPercent(-35), 1)
        tank_drive.on_for_degrees(SpeedPercent(0), SpeedPercent(40), 300)
        drive()
    elif ts_left.is_pressed:
        tank_drive.stop()
        s.beep()
        tank_drive.on_for_rotations(SpeedPercent(-35), SpeedPercent(-35), 1)
        tank_drive.on_for_degrees(SpeedPercent(40), SpeedPercent(0), 300)
        drive()
