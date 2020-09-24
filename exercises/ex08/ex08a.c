#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <alchemy/sem.h>
#include <alchemy/task.h>

#define MEASURECOUNT 10000

RT_SEM mysync;

void measureJitter(void *arg) {
    RTIME* wakeTimes = (RTIME *)arg;

    for (unsigned int i = 0; i < MEASURECOUNT; ++i) {
        wakeTimes[i] = rt_timer_read();
        rt_task_wait_period(NULL);
    }

    rt_sem_v(&mysync);

    return;
}

void calc_time_diffs(RTIME* wakeTimes, RTIME* timeDiffs) {
    for (unsigned int i = 0; i < MEASURECOUNT - 1; ++i) {
        timeDiffs[i] = wakeTimes[i + 1] - wakeTimes[i];
    }
}

void write_RTIMES(char * filename, RTIME *timeValues, unsigned int size){
    FILE *file;
    file = fopen(filename, "w");

    for (unsigned int i = 0; i < size; ++i) {
        fprintf(file, "%u,%llu\n", i, timeValues[i]);
    }

    fclose(file);
}

RTIME calc_average_time(RTIME* timeDiffs, unsigned int size) {
    RTIME average = 0;
    for (unsigned int i = 0; i < size; ++i) {
        average += timeDiffs[i];
    }

    return average / size;
}

int main(int argc, char* argv[])
{
    RTIME wakeTimes[MEASURECOUNT];
    RTIME timeDiffs[MEASURECOUNT - 1];
    RT_TASK jitterTask;
    RTIME average;

    rt_sem_create(&mysync, "mySync", 0, S_FIFO);
    rt_task_create(&jitterTask, "jitterTask", 0, 50, 0);
    rt_task_set_periodic(&jitterTask, TM_NOW, 100e3);
    rt_task_start(&jitterTask, &measureJitter, wakeTimes);

    rt_sem_p(&mysync,TM_INFINITE);

    calc_time_diffs(wakeTimes, timeDiffs);
    write_RTIMES("ex08a.csv", timeDiffs, MEASURECOUNT - 1);

    average = calc_average_time(timeDiffs, MEASURECOUNT - 1);
    printf("average  %llu\n", average);
}