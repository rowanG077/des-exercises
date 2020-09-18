#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <alchemy/task.h>
#include <alchemy/sem.h>
#include <alchemy/timer.h>

#define SPINTIME   1e7
#define TIMEUNIT   5e7

RT_TASK lowPrio;
RT_TASK medPrio;
RT_TASK highPrio;
RT_SEM mysync;

void high(void *arg) {

    for (int i = 0; i < 3; ++i) {
      printf("High priority task tries to lock semaphore\n");
      rt_sem_p(&mysync,TM_INFINITE);
      printf("High priority task locks semaphore\n");
      printf("High priority task unlocks semaphore\n");
      rt_sem_v(&mysync);
    }

    printf("..........................................High priority task ends\n");
}

void med(void *arg) {
    for (int i = 0; i < 12; ++i) {
      if (i == 2) {
    			rt_task_start(&highPrio, &high, NULL);
      }
      printf("Medium task running\n");
    }
    printf("..........................................Medium priority task ends\n");
}

void low(void *arg) {

    for (int i = 0; i < 3; ++i) {
      rt_sem_p(&mysync,TM_INFINITE);
      printf("Low priority task locks semaphore\n");
      if (i == 0) {
          rt_task_start(&medPrio, &med, NULL);
      }
      printf("Low priority task unlocks semaphore\n");
      rt_sem_v(&mysync);
    }

    printf("..........................................Low priority task ends\n");
}

//startup code
void startup() {
    int i;
    // semaphore to sync task startup on
    rt_sem_create(&mysync, "MySemaphore", 1, S_FIFO);
      		rt_task_create(&highPrio, "high_task", 0, 70, 0);
          rt_task_create(&medPrio, "med_task", 0, 60, 0);
    rt_task_create(&lowPrio, "low_task", 0, 50, 0);
    rt_task_start(&lowPrio, &low, NULL);
}

int main(int argc, char* argv[])
{
  startup();
  printf("\nType CTRL-C to end this program\n\n" );
  pause();
}
