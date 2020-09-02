#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <alchemy/task.h>

#define NTASKS 5
RT_TASK task[NTASKS];
int indices[NTASKS] = {0};

void demo(void *arg) {
    RT_TASK_INFO curtaskinfo;
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
    rt_task_create(&task[i], taskName, 0, 50 + i, 0);
    rt_task_start(&task[i], &demo, &indices[i]);
  }

}