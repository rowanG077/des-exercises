package ev3.behaviours.generator

import ev3.behaviours.ev3DSL.Missions

class PythonMasterGenerator {
	def static toPython(Missions ms)'''
	#!/usr/bin/env python3

	import argparse

	from ev3dev2.motor import Motor, MoveTank, OUTPUT_A, OUTPUT_B, OUTPUT_D
	from ev3dev2._platform.ev3 import INPUT_3, INPUT_2, INPUT_4, INPUT_1
	from ev3dev2.sensor.lego import ColorSensor, TouchSensor, UltrasonicSensor
	from ev3dev2.sound import Sound

	from robot import Robot

	«FOR m : ms.missions»
	«FOR b : m.behaviours»
	from «m.name».«b.name» import «b.name» as «m.name»«b.name»
	«ENDFOR»
	«ENDFOR»

	def main():
	    tank = MoveTank(OUTPUT_A, OUTPUT_D)
	    probe = Motor(OUTPUT_B)
	    sound = Sound()

	    cs_left = ColorSensor(INPUT_1)
	    cs_middle = ColorSensor(INPUT_2)
	    cs_right = ColorSensor(INPUT_3)
	    us_back = UltrasonicSensor(INPUT_4)
	    us_back.mode = UltrasonicSensor.MODE_US_DIST_CM

	    robot = Robot("00:17:E9:B4:CE:E6", tank, sound)
	    «FOR m : ms.missions»
	    «FOR b : m.behaviours»
	    robot.add_mission_behaviour("«m.name»", «m.name»«b.name»(robot, tank, probe, cs_left, cs_middle, cs_right, us_back))
	    «ENDFOR»
	    «ENDFOR»
	    robot.run()

	    print("Robot stopped")

	if __name__ == '__main__':
	    main()
	'''
}
