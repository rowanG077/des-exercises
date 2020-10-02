#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>

#include <alchemy/sem.h>
#include <alchemy/task.h>
#include <rtdm/gpio.h>

#define MEASURECOUNT 50

RT_SEM finished;
RTIME spinTimes[MEASURECOUNT] = {0};
RTIME eclipseTimes[MEASURECOUNT] = {0};

void close_gpio(int fd) {
    if (close(fd) == -1) {
        printf("Could not close gpio file descriptor.\n");
        exit(1);
    }
}

int open_lightsensor_gpio(void) {
    int xeno_trigger = GPIO_TRIGGER_EDGE_FALLING | GPIO_TRIGGER_EDGE_RISING;

    int fd = open("/dev/rtdm/pinctrl-bcm2835/gpio23", O_RDONLY);
    if (fd == -1) {
        printf("Could not open lightsensor (gpio23) device file.\n");
        exit(1);
    }
    if (ioctl(fd, GPIO_RTIOC_IRQEN, &xeno_trigger) == -1) {
        printf("Could not do GPIO_RTIOC_IRQEN on lightsensor (gpio23) file descriptor.\n");
        exit(1);
    }
    return fd;
}

void write_RTIMES(char * filename, RTIME *spinTimes, RTIME *eclipseTimes, size_t size){
    FILE *file;
    file = fopen(filename, "w");

    fprintf("index;spinTime(ns);eclipseTime(ns)\n");
    for (size_t i = 0; i < size; ++i) {
        fprintf(file, "%u;%llu;%llu\n", i, spinTimes[i], eclipseTimes[i]);
    }

    fclose(file);
}

void measure(void* arg) {
    size_t spinTimesIndex = 0;
    size_t eclipseTimesIndex = 0;

    int lightsensor = open_lightsensor_gpio();
    int lightsensorValue = 0;
    RTIME currentT = rt_timer_read();
    RTIME lastFallingTriggerT = currentT;
    RTIME lastRisingTriggerT = currentT;

    while (eclipseTimesIndex < MEASURECOUNT || spinTimesIndex < MEASURECOUNT) {
        if (read(lightsensor, &lightsensorValue, sizeof(lightsensorValue)) != sizeof(lightsensorValue)) {
            printf("Error while reading from light sensor.\n");
            exit(1);
        }
        currentT = rt_timer_read();
        // We left the light area in the disk
        if (lightsensorValue == 0 && eclipseTimesIndex < MEASURECOUNT) {
            eclipseTimes[eclipseTimesIndex++] = currentT - lastRisingTriggerT;
            lastFallingTriggerT = currentT;
        }
        else if (lightsensorValue == 1 && spinTimesIndex < MEASURECOUNT) {
            spinTimes[spinTimesIndex++] = currentT - lastRisingTriggerT;
            lastRisingTriggerT = currentT;
        }
    }


    close_gpio(lightsensor);
    rt_sem_v(&finished);

    return;
}

int main(int argc, char* argv[])
{
    RT_TASK measureTask;

    rt_sem_create(&finished, "finished", 0, S_FIFO);
    rt_task_create(&measureTask, "measureTask", 0, 50, 0);
    rt_task_start(&measureTask, &measure, NULL);

    rt_sem_p(&finished, TM_INFINITE);

    write_RTIMES("09a.csv", spinTimes, eclipseTimes, MEASURECOUNT);

    return 0;
}