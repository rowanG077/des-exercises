package ev3.behaviours.generator

import ev3.behaviours.ev3DSL.Behaviour
import ev3.behaviours.ev3DSL.SetStatement
import ev3.behaviours.ev3DSL.Boolean
import ev3.behaviours.ev3DSL.Colors
import ev3.behaviours.ev3DSL.BooleanExpressionLevelOr
import ev3.behaviours.ev3DSL.BooleanExpressionLevelAnd
import ev3.behaviours.ev3DSL.BooleanBracket
import ev3.behaviours.ev3DSL.NotExpression
import ev3.behaviours.ev3DSL.ColorSensorReading
import ev3.behaviours.ev3DSL.UltrasonicSensorReading
import ev3.behaviours.ev3DSL.CompareOperator
import ev3.behaviours.ev3DSL.TouchSensorReading
import ev3.behaviours.ev3DSL.TouchSensorSide
import ev3.behaviours.ev3DSL.Variable
import ev3.behaviours.ev3DSL.Conditional
import ev3.behaviours.ev3DSL.Block
import ev3.behaviours.ev3DSL.DriveStatement
import ev3.behaviours.ev3DSL.DriveDirection
import ev3.behaviours.ev3DSL.TurnDirection
import ev3.behaviours.ev3DSL.TurnStatement
import ev3.behaviours.ev3DSL.WaitStatement

class PythonGenerator {
	def static toPython(Behaviour b)'''
	from threading import Lock
	import time
	from ev3dev2.sensor.lego import ColorSensor, UltrasonicSensor
	from ev3dev2.motor import SpeedPercent
	from ev3dev2.led import Leds

	class «b.name»(Behaviour):
		def __init__(self, robot, tank, color_sensor, touch_left, touch_right, ultrasonic_sensor):
			super().__init__(robot, «b.prio»)
			self.tank = tank
			self.color_sensor = color_sensor
			self.touch_left = touch_left
			self.touch_right = touch_right
			self.ultrasonic_sensor = ultrasonic_sensor

			# State init
			«FOR s : b.states»
			«genStatement(s, "")»
			«ENDFOR»

		def take_control(self):
			return super().take_control() or («generateBoolExp(b.when)»)

		def action_gen(self):
«genBlock(b.getDo(), "\t\t")»
	'''

	def static dispatch generateBoolExp(BooleanExpressionLevelOr exp)
		'''«generateBoolExp(exp.left)» or «generateBoolExp(exp.right)»'''

	def static dispatch generateBoolExp(BooleanExpressionLevelAnd exp)
		'''«generateBoolExp(exp.left)» and «generateBoolExp(exp.right)»'''

	def static dispatch generateBoolExp(NotExpression exp)
		'''not «generateBoolExp(exp.expr)»'''

	def static dispatch generateBoolExp(BooleanBracket exp)
		'''(«generateBoolExp(exp.sub)»)'''

	def static dispatch generateBoolExp(ColorSensorReading r)
		'''self.color_sensor.color == «genColor(r.color)»'''

	def static dispatch generateBoolExp(UltrasonicSensorReading r)
		'''self.ultrasonic_sensor.value() «generateOp(r.op)» «r.distance»'''

	def static dispatch generateBoolExp(TouchSensorReading r)
		'''«genTouchSensorSide(r.side)»'''

	def static dispatch generateBoolExp(Variable v)
		'''self.«v.variable»'''

	def static genBlock(Block b, String indent) {
		var result = "";
		for (s : b.statements) {
			result += genStatement(s, indent) + "\n";
		}
		return result.substring(0, result.length() - 1);
	}

	def static dispatch genStatement(Conditional stmt, String indent)'''
	«indent»if («generateBoolExp(stmt.condition)»):
	«genBlock(stmt.block, indent + "\t")»
	«FOR e : stmt.elseifs»
	«indent»elif («generateBoolExp(e.condition)»):
	«genBlock(e.block, indent + "\t")»
	«ENDFOR»
	«IF stmt.elseBlock !==  null»
	«indent»else:
	«genBlock(stmt.elseBlock, indent + "\t")»
	«ENDIF»
	'''

	def static dispatch genStatement(DriveStatement stmt, String indent) {
		var leftSpeed = stmt.direction == DriveDirection.FORWARD ? stmt.speed : -stmt.speed;
		var rightSpeed = stmt.direction == DriveDirection.FORWARD ? stmt.speed : -stmt.speed;
		if (stmt.withStatement !== null) {
			val turnFrac = stmt.withStatement.turnSpeed / 100.0;
			if (stmt.withStatement.turnDirection == TurnDirection.LEFT) {
				rightSpeed = (leftSpeed - (turnFrac * leftSpeed)).intValue();
			}
			else if (stmt.withStatement.turnDirection == TurnDirection.RIGHT) {
				leftSpeed = (rightSpeed - (turnFrac * rightSpeed)).intValue();
			}
		}

		return '''«indent»self.tank.on(SpeedPercent(«leftSpeed»), SpeedPercent(«rightSpeed»))'''
	}

	def static dispatch genStatement(TurnStatement stmt, String indent) {
		var leftSpeed = stmt.direction == TurnDirection.LEFT ? 50 : -50;
		var rightSpeed = stmt.direction == TurnDirection.LEFT? -50 : 50;
		return '''
		«indent»self.tank.on_for_degrees(SpeedPercent(«leftSpeed»), SpeedPercent(«rightSpeed»), degrees=«stmt.degrees», block=False)
		«indent»while self.tank.is_running:
		«indent»	yield
		'''
	}

	def static dispatch genStatement(WaitStatement stmt, String indent) {
		return '''
		«indent»t1 = time.clock()
		«indent»while (time.clock() - t1) < «stmt.duration»:
		«indent»	yield
		'''
	}


	def static dispatch genStatement(SetStatement stmt, String indent)
		'''«indent»self.«stmt.variable» = «genBoolean(stmt.value)»'''

	def static genBoolean(Boolean b) {
		switch (b) {
		case Boolean.TRUE:
			return "True"
		case Boolean.FALSE:
			return "False"
		}
	}

	def static genTouchSensorSide(TouchSensorSide s) {
		switch (s) {
		case TouchSensorSide.LEFT:
			return "self.touch_left.is_pressed"
		case TouchSensorSide.RIGHT:
			return "self.touch_right.is_pressed"
		}
	}

	def static genColor(Colors c) {
		switch (c) {
		case Colors.RED:
			return "ColorSensor.COLOR_RED"
		case Colors.BLUE:
			return "ColorSensor.COLOR_BLUE"
		case Colors.YELLOW:
			return "ColorSensor.COLOR_YELLOW"
		case Colors.BLACK:
			return "ColorSensor.COLOR_BLACK"
		}
	}

	def static generateOp(CompareOperator op) {
		switch (op) {
		case CompareOperator.EQ:
			return "=="
		case CompareOperator.NEQ:
			return "!="
		case CompareOperator.LE:
			return "<="
		case CompareOperator.GE:
			return ">="
		case CompareOperator.L:
			return "<"
		case CompareOperator.G:
			return ">"
		}
	}
}
