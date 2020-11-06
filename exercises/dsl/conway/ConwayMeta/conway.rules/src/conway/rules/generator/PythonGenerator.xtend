package conway.rules.generator

import conway.rules.rulesDSL.Rule
import conway.rules.rulesDSL.BothRule
import conway.rules.rulesDSL.ConstantExpression
import conway.rules.rulesDSL.CellState
import conway.rules.rulesDSL.BooleanExpression
import conway.rules.rulesDSL.BooleanExpressionLevelOr
import conway.rules.rulesDSL.AliveDeadRule
import conway.rules.rulesDSL.BooleanExpressionLevelAnd
import conway.rules.rulesDSL.LeftNeighbourComparison
import conway.rules.rulesDSL.RightNeighbourComparison
import conway.rules.rulesDSL.CompareOperator
import conway.rules.rulesDSL.BooleanBracket
import java.util.Optional

class PythonGenerator {	
	def static toPython(Rule r)'''
		# Generated for «r.name»
		def apply_rules(current_value, total, on_value, off_value):
			«generateStateSet(r.states)»
	'''

	def static dispatch generateStateSet(BothRule stateRule) {
		return generateBoolExpOrConstant(stateRule.bothRule.condition, Optional.empty())
	}
	
	def static dispatch generateStateSet(AliveDeadRule stateRule)'''
		if current_value == on_value:
			«generateBoolExpOrConstant(stateRule.aliveRule.condition, Optional.of('''off_value'''))»
		else if current_value == off_value:
			«generateBoolExpOrConstant(stateRule.deadRule.condition, Optional.of('''on_value'''))»
		return current_value
	'''
	
	def static dispatch generateBoolExpOrConstant(BooleanExpression c, Optional<CharSequence> knownValue)'''
		if «generateBoolExp(c)»:
			«IF knownValue.isPresent()»
				return «knownValue.get()»
			«ELSE»
				return on_value if current_value == off_value else off_value
			«ENDIF»
	'''
	
	def static dispatch generateBoolExpOrConstant(ConstantExpression c, Optional<CharSequence> _) {
		switch (c.constant) {
			case CellState.ALIVE:
				return "return on_value"
			case CellState.DEAD:
				return "return off_value"
		}
	}
	
	def static dispatch generateBoolExp(BooleanExpressionLevelOr exp)
		'''«generateBoolExp(exp.left)» or «generateBoolExp(exp.right)»'''
		
	def static dispatch generateBoolExp(BooleanExpressionLevelAnd exp)
		'''«generateBoolExp(exp.left)» and «generateBoolExp(exp.right)»'''
		
	def static dispatch generateBoolExp(BooleanBracket exp)
		'''(«generateBoolExp(exp.sub)»)'''
		
	def static dispatch generateBoolExp(LeftNeighbourComparison exp)
		'''total «generateOp(exp.op)» «exp.value»'''
		
	def static dispatch generateBoolExp(RightNeighbourComparison exp)
		'''«exp.value» «generateOp(exp.op)» total'''
		
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