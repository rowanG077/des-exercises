package conway.rules.generator

import conway.rules.rulesDSL.Rule

class PythonGenerator {
	def static toPython(Rule root)'''
		# Generated for «root.name»
		def apply_rules(current_value, total, on_value, off_value):
		    if current_value == on_value:
		        if total < 2 or total > 3:
		            return off_value
		    else:
		        if total == 3:
		            return on_value
		    return current_value
	'''
}