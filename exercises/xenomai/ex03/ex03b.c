#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <alchemy/task.h>
#include <alchemy/timer.h>
#include <alchemy/sem.h>

#define NTASKS 5

static RT_TASK t1[NTASKS];
static RT_TASK t2[NTASKS];

int global;
RT_SEM sem1;
RT_SEM sem2;

void taskOne(void *arg)
{
    int num = * (int *)arg;
    while (1) {
        rt_sem_p(&sem1, TM_INFINITE);
        global = 0;
        printf("I am taskOne num %d and global = %d\n", num, global);
        rt_sem_v(&sem2);
    }
}

void taskTwo(void *arg)
{
    int num = * (int *)arg;
    while (1) {
        rt_sem_p(&sem2, TM_INFINITE);
        global = 1;
        printf("I am taskTwo num %d and global = %d\n", num, global);
        rt_sem_v(&sem1);
    }
}

int main(int argc, char* argv[]) {

    char taskName[20];

    int indices[NTASKS] = {0};
    rt_sem_create(&sem1, "exercise 3 - 1", 1, S_PRIO);
    rt_sem_create(&sem2, "exercise 3 - 2", 0, S_PRIO);

    for (unsigned i = 0; i < NTASKS; ++i) {
        indices[i] = i;
        sprintf(taskName, "Task One %u", i);
        rt_task_create(&t1[i], taskName, 0, 0, 0);
        sprintf(taskName, "Task Two %u", i);
        rt_task_create(&t2[i], taskName, 0, 0, 0);
        rt_task_start(&t1[i], &taskOne, &indices[i]);
        rt_task_start(&t2[i], &taskTwo, &indices[i]);
    }

    pause();

    return 0;
}