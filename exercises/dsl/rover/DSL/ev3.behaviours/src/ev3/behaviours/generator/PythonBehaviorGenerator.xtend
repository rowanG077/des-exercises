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
import ev3.behaviours.ev3DSL.ColorSensorSide
import ev3.behaviours.ev3DSL.UltrasonicSensorSide
import ev3.behaviours.ev3DSL.StopStatement
import ev3.behaviours.ev3DSL.ProbeStatement
import org.eclipse.emf.common.util.EList
import ev3.behaviours.ev3DSL.WhileStatement
import java.util.List
import java.util.ArrayList

class PythonBehaviorGenerator {
	def static toPython(Behaviour b)'''
	from threading import Lock
	import time
	from ev3dev2.sensor.lego import ColorSensor, UltrasonicSensor
	from ev3dev2.motor import SpeedPercent
	from ev3dev2.led import Leds

	from base_behaviour import BaseBehaviour
	from robot import RemoteSensor

	class «b.name»(BaseBehaviour):
	    def __init__(self, robot, tank, probe, cs_left, cs_middle, cs_right, us_back):
	        super().__init__(robot, «b.prio»)
	        self.tank = tank
	        self.probe = probe
	        self.cs_left = cs_left
	        self.cs_middle = cs_middle
	        self.cs_right = cs_right
	        self.us_back = us_back

	        # State init
	        «generateStateInit(b.states)»

	    def take_control(self):
	        return super().take_control() or («generateBoolExp(b.when)»)

	    def action_gen(self):
	        yield
	        «genBlock(b.getDo())»
	'''

	def static generateStateInit(EList<SetStatement> states)'''
	«FOR s : states»
    «genStatement(s)»
	«ENDFOR»
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
		'''(«FOR s : genColorSensorSide(r.side) SEPARATOR " or "»«s» == «genColor(r.color)»«ENDFOR»)'''

	def static dispatch generateBoolExp(UltrasonicSensorReading r)
		'''«genUltrasonicSensorSide(r.side)» «generateOp(r.op)» «r.distance»'''

	def static dispatch generateBoolExp(TouchSensorReading r)
		'''(«FOR s : genTouchSensorSide(r.side) SEPARATOR " or "»«s»«ENDFOR»)'''

	def static dispatch generateBoolExp(Variable v)
		'''self.«v.variable»'''

	def static genBlock(Block b) {
		var result = "";
		for (s : b.statements) {
			result += genStatement(s) + "\n";
		}
		return result.substring(0, result.length() - 1);
	}

	def static dispatch genStatement(Conditional stmt)'''
	if («generateBoolExp(stmt.condition)»):
	    «genBlock(stmt.block)»
	«FOR e : stmt.elseifs»
	elif («generateBoolExp(e.condition)»):
	    «genBlock(e.block)»
	«ENDFOR»
	«IF stmt.elseBlock !==  null»
	else:
	    «genBlock(stmt.elseBlock)»
	«ENDIF»
	'''

	def static dispatch genStatement(DriveStatement stmt) {
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

		return '''
		self.tank.on(SpeedPercent(«leftSpeed»), SpeedPercent(«rightSpeed»))
		«IF stmt.forStatement !==  null»
		«genWait(stmt.forStatement.duration)»
		«ENDIF»
		'''
	}

	def static dispatch genStatement(TurnStatement stmt) {
		var leftSpeed = stmt.direction == TurnDirection.LEFT ? -50 : 50;
		var rightSpeed = stmt.direction == TurnDirection.LEFT? 50 : -50;
		var multiplier = leftSpeed != 0 ? 100 / Math.abs(leftSpeed) : 0;
		return '''
		self.tank.on_for_degrees(SpeedPercent(«leftSpeed»), SpeedPercent(«rightSpeed»), degrees=«stmt.degrees * multiplier», block=False)
		while self.tank.is_running:
		    yield
		'''
	}

	def static dispatch genStatement(WaitStatement stmt) {
		return genWait(stmt.duration);
	}

	def static dispatch genStatement(SetStatement stmt)
		'''self.«stmt.variable» = «genBoolean(stmt.value)»'''

	def static dispatch genStatement(StopStatement stmt)'''
		self.tank.off()
		yield
		'''

	def static dispatch genStatement(ProbeStatement stmt)'''
		self.probe.on_for_degrees(10,-90,brake=True, block=False)
		while self.probe.is_running:
		    yield
		self.probe.on_for_degrees(15,90, brake=True, block=False)
		while self.probe.is_running:
		    yield
		'''

	def static dispatch genStatement(WhileStatement stmt)'''
		while («generateBoolExp(stmt.condition)»):
		    «genBlock(stmt.block)»
		    yield
		'''

	def static genBoolean(Boolean b) {
		switch (b) {
		case Boolean.TRUE:
			return "True"
		case Boolean.FALSE:
			return "False"
		}
	}

	def static genWait(int seconds)'''
		t1 = time.time()
		while (time.time() - t1) < «seconds»:
		    yield
		'''

	def static genTouchSensorSide(TouchSensorSide s) {
		var sensors = new ArrayList<CharSequence>();
		if (s === TouchSensorSide.LEFT  || s === TouchSensorSide.ANY) {
			sensors.add("self.robot.get_sensordata(RemoteSensor.TS_LEFT)")
		}
		if (s === TouchSensorSide.RIGHT  || s === TouchSensorSide.ANY) {
			sensors.add("self.robot.get_sensordata(RemoteSensor.TS_RIGHT)")
		}
		if (s === TouchSensorSide.BACK  || s === TouchSensorSide.ANY) {
			sensors.add("self.robot.get_sensordata(RemoteSensor.TS_BACK)")
		}
		return sensors
	}

	def static genColorSensorSide(ColorSensorSide s) {
		var sensors = new ArrayList<CharSequence>();
		if (s === ColorSensorSide.LEFT  || s === ColorSensorSide.ANY) {
			sensors.add("self.cs_left.color")
		}
		if (s === ColorSensorSide.RIGHT  || s === ColorSensorSide.ANY) {
			sensors.add("self.cs_right.color")
		}
		if (s === ColorSensorSide.MIDDLE  || s === ColorSensorSide.ANY) {
			sensors.add("self.cs_middle.color")
		}
		return sensors
	}

	def static genUltrasonicSensorSide(UltrasonicSensorSide s) {
		switch (s) {
		case UltrasonicSensorSide.FRONT:
			return "self.robot.get_sensordata(RemoteSensor.US_FRONT)"
		case UltrasonicSensorSide.BACK:
			return "self.us_back.value()"
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
		case Colors.WHITE:
			return "ColorSensor.COLOR_WHITE"
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
