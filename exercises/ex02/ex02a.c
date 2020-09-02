#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <alchemy/task.h>

#define NTASKS 5
RT_TASK task[NTASKS];

void demo(void *arg) {
    RT_TASK_INFO curtaskinfo;
    rt_task_inquire(NULL,&curtaskinfo);
    rt_printf("Task name: %s", curtaskinfo.name);
}

int main(int argc, char* argv[])
{
  char taskName[10];

  for (unsigned i = 0; i < NTASKS; ++i) {
    printf("start task %u\n", i);
    sprintf(taskName, "Task %u", i);
    rt_task_create(&task[i], taskName, 0, 50, 0);
    rt_task_start(&task[i], &demo, 0);
  }
}