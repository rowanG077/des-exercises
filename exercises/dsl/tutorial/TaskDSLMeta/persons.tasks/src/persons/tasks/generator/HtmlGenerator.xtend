package persons.tasks.generator

import persons.tasks.taskDSL.Planning
import persons.tasks.taskDSL.LunchAction
import persons.tasks.taskDSL.MeetingAction
import persons.tasks.taskDSL.PaperAction
import persons.tasks.taskDSL.PaymentAction
import persons.tasks.taskDSL.Task

class HtmlGenerator {	
	def static toHtml(Planning root)'''
		<html>
		<head>
		<style>
		table, th, td
		{ border:1px solid black;
		padding:5px;}
		table
		{ border-spacing:15px; }
		</style>
		</head>
		<body>
		<H1>Planning: «root.name»</H1>
		«listTasks(root)»
		</body>
		</html>'''
	
	def static listPersons(Task task)'''
		«FOR p : task.person SEPARATOR " , " AFTER " "» «p.name» «ENDFOR»'''
		
	def static listTasks(Planning root)'''
		<table style="width:300px">
		<tr>
		<th>Persons</th>
		<th>Action</th>
		<th>Priority</th>
		</tr>
		«FOR t: root.tasks»
		<tr>
		<td>«IF (!root.anonymous)»«listPersons(t)»«ENDIF»</td>
		<td>«action2Html(t.action)»</td>
		<td>«t.prio»</td>
		</tr>
		«ENDFOR»
		</table>'''
				
	
	def static dispatch action2Html(LunchAction action)'''
		Lunch at «action.location»'''
	
	def static dispatch action2Html(MeetingAction action)'''
		Meeting '''
	
	def static dispatch action2Html(PaperAction action)'''
		Paper for «action.report»'''
	
	def static dispatch action2Html(PaymentAction action)'''
		Pay'''
		
}