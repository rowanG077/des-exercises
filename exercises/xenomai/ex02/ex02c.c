#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <alchemy/task.h>

#define NTASKS 5
RT_TASK task[NTASKS];
int indices[NTASKS] = {0};
const RTIME sec = 1e9;

void demo(void *arg) {
    RT_TASK_INFO curtaskinfo;
    rt_task_sleep(sec);
    int num = * (int *)arg;
    rt_task_inquire(NULL,&curtaskinfo);
    rt_printf("Task name: %s, number: %d\n", curtaskinfo.name, num);
}

int main(int argc, char* argv[])
{
  char taskName[10];

  for (int i = 0; i < NTASKS; ++i) {
    indices[i] = i;
    printf("start task %i\n", i);
    sprintf(taskName, "Task %i", i);
    rt_task_create(&task[i], taskName, 0, 50, 0);
    rt_task_start(&task[i], &demo, &indices[i]);
  }

  printf("End program by CTRL-C\n");
  pause();
}