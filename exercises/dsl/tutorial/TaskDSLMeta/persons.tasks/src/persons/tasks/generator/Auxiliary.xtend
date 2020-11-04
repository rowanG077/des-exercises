package persons.tasks.generator

import java.util.List
import persons.tasks.taskDSL.Action
import persons.tasks.taskDSL.Planning

class Auxiliary {
	def static List<Action> getActions(Planning root)
	{
		return root.tasks.map[t|t.action];
	}
}