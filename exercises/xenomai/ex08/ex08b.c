#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <alchemy/sem.h>
#include <alchemy/task.h>
#include <rtdm/gpio.h>

#define MEASURECOUNT 10000

RT_SEM mysync;
RT_SEM interruptSync;

int open_led_gpio(void) {
    int value = 0;
    int fd = open("/dev/rtdm/pinctrl-bcm2835/gpio22",O_WRONLY);
    if (fd == -1) {
        printf("Could not open LED (gpio22) device file.\n");
        exit(1);
    }
    //set device to output mode with special device request GPIO_RTIOC_DIR_OUT
    if (ioctl(fd, GPIO_RTIOC_DIR_OUT, &value) == -1) {
        printf("Could not do GPIO_RTIOC_DIR_OUT on LED (gpio22) file descriptor.\n");
        exit(1);
    }
    return fd;
}

void close_gpio(int fd) {
    if (close(fd) == -1) {
        printf("Could not close gpio file descriptor.\n");
        exit(1);
    }
}

int open_led_read_gpio(void) {
    int xeno_trigger = GPIO_TRIGGER_EDGE_FALLING | GPIO_TRIGGER_EDGE_RISING;

    int fd = open("/dev/rtdm/pinctrl-bcm2835/gpio24", O_RDONLY);
    if (fd == -1) {
        printf("Could not open LED read (gpio24) device file.\n");
        exit(1);
    }
    if (ioctl(fd, GPIO_RTIOC_IRQEN, &xeno_trigger) == -1) {
        printf("Could not do GPIO_RTIOC_IRQEN on BUTTON (gpio23) file descriptor.\n");
        exit(1);
    }
    return fd;
}


void pinToggler(void* arg) {
    RTIME* toggleTimes = (RTIME*) arg;
    int led_state = 1;
    int led = open_led_gpio();

    rt_sem_p(&interruptSync, TM_INFINITE);
    rt_task_sleep(50e6);

    rt_task_set_periodic(NULL, TM_NOW, 1e6);

    for (unsigned int i = 0; i < MEASURECOUNT; ++i) {
        toggleTimes[i] = rt_timer_read();
        if (write(led, &led_state, sizeof(led_state)) != sizeof(led_state)) {
            printf("Error while writing to led.\n");
            exit(1);
        }
        led_state = led_state == 0 ? 1 : 0;
        rt_task_wait_period(NULL);
    }

    close_gpio(led);

    return;
}

void interruptReader(void* arg) {
    RTIME* interruptTimes = (RTIME*) arg;
    int led_read = open_led_read_gpio();
    int value = 0;

    rt_sem_v(&interruptSync);

    for (unsigned int i = 0; i < MEASURECOUNT; ++i) {
        if (read(led_read, &value, sizeof(value)) != sizeof(value)) {
            printf("Error while reading led interrupt.\n");
            exit(1);
        }
        interruptTimes[i] = rt_timer_read();
    }

    close_gpio(led_read);

    rt_sem_v(&mysync);

    return;
}

void worker(void* arg) {
    printf("Start working \n");
    while (1) {
        rt_task_sleep(200e3);
        printf(".");
        rt_timer_spin(500e3);
    }
}

void calc_time_diffs(RTIME* toggleTimes, RTIME* interruptTimes, RTIME* timeDiffs) {
    for (unsigned int i = 0; i < MEASURECOUNT; ++i) {
        timeDiffs[i] = interruptTimes[i] - toggleTimes[i];
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
    RTIME toggleTimes[MEASURECOUNT];
    RTIME interruptTimes[MEASURECOUNT];
    RTIME timeDiffs[MEASURECOUNT];

    RT_TASK togglerTask;
    RT_TASK workerTask;
    RT_TASK interruptTask;

    RTIME average;

    rt_sem_create(&mysync, "mySync", 0, S_FIFO);
    rt_sem_create(&interruptSync, "interruptSync", 0, S_FIFO);

    rt_task_create(&togglerTask, "togglerTask", 0, 40, 0);
    rt_task_create(&workerTask, "workerTask", 0, 30, 0);
    rt_task_create(&interruptTask, "interruptTask", 0, 50, 0);

    rt_task_start(&interruptTask, &interruptReader, interruptTimes);
    rt_task_start(&togglerTask, &pinToggler, toggleTimes);
    rt_task_start(&workerTask, &worker, NULL);

    rt_sem_p(&mysync, TM_INFINITE);

    calc_time_diffs(toggleTimes, interruptTimes, timeDiffs);
    write_RTIMES("ex08b.csv", timeDiffs, MEASURECOUNT);

    average = calc_average_time(timeDiffs, MEASURECOUNT);
    printf("average  %llu\n", average);
}
