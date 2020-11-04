package persons.tasks.generator

import persons.tasks.taskDSL.Planning
import persons.tasks.taskDSL.MeetingAction
import persons.tasks.taskDSL.LunchAction
import persons.tasks.taskDSL.PaperAction
import persons.tasks.taskDSL.PaymentAction
import persons.tasks.taskDSL.Task
import persons.tasks.taskDSL.TimeUnit

class TextGenerator {
	def static toText(Planning root) '''
		Info of the planning «root.name»
		All Persons:«"\n"»
		«FOR p : root.persons»«"\t"»«p.name»«"\n"»«ENDFOR»
		All actions of tasks:
		«FOR t : root.tasks BEFORE "====== \n" SEPARATOR "," AFTER "====="»
		«action2Text(t.action)»«infoAction(t)»
		«ENDFOR»
		Other way of listing all tasks:
		«FOR a : Auxiliary.getActions(root) SEPARATOR " , "»
		«action2Text(a)»
		«ENDFOR»'''

	def static dispatch action2Text(LunchAction action) '''
		Lunch at location «action.location»'''

	def static dispatch action2Text(MeetingAction action) '''
		Meeting with topic «action.topic»'''

	def static dispatch action2Text(PaperAction action) '''
		Paper for journal «action.report»'''

	def static dispatch action2Text(PaymentAction action) '''
		Pay «action.amount» euro'''

	def static infoAction(Task t) '''
		«IF t.duration !== null» with duration: «t.duration.dl» «toText(t.duration.unit)»«ENDIF»'''

	def static CharSequence toText(TimeUnit u) {
		switch (u) {
			case TimeUnit::MINUTE: return '''m'''
			case TimeUnit::HOUR: return '''h'''
			case TimeUnit::DAY: return '''d'''
			case TimeUnit::WEEK: return '''w'''
		}
	}

}
