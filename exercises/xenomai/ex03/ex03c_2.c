#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <alchemy/task.h>
#include <alchemy/sem.h>

#define NTASKS 5
RT_TASK task[NTASKS];
int indices[NTASKS] = {0};
RT_SEM sem;

void demo(void *arg) {
    RT_TASK_INFO curtaskinfo;
    int num = * (int *)arg;
    rt_sem_p(&sem, TM_INFINITE);
    rt_task_inquire(NULL,&curtaskinfo);
    rt_printf("Task name: %s, number: %d\n", curtaskinfo.name, num);
}

int main(int argc, char* argv[])
{
  char taskName[10];

  rt_sem_create(&sem, "exercise 3", 0, S_PRIO);

  for (int i = 0; i < NTASKS; ++i) {
    indices[i] = i;
    printf("start task %i\n", i);
    sprintf(taskName, "Task %i", i);
    rt_task_create(&task[i], taskName, 0, 50 + i, S_PRIO);
    rt_task_start(&task[i], &demo, &indices[i]);
  }

  rt_sem_broadcast(&sem);
}