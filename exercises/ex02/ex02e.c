#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <alchemy/task.h>

#define NTASKS 3
RT_TASK task[NTASKS];
int indices[NTASKS] = {0};

void demo(void *arg) {
    RT_TASK_INFO curtaskinfo;
    RTIME num = * (int *)arg;
    rt_task_inquire(NULL,&curtaskinfo);
    while (1) {
      rt_printf("Running task name: %s, number: %d\n", curtaskinfo.name, num);
      rt_task_wait_period(NULL);
    }
    return;
}

int main(int argc, char* argv[])
{
  char taskName[10];

  for (int i = 0; i < NTASKS; ++i) {
    indices[i] = i;
    printf("start task %i\n", i);
    sprintf(taskName, "Task %i", i);
    rt_task_create(&task[i], taskName, 0, 50, 0);
    rt_task_set_periodic(&task[i], TM_NOW, 1e9 * (i + 1));
    rt_task_start(&task[i], &demo, &indices[i]);
  }

  pause();
}