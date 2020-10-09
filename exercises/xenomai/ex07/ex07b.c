#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <alchemy/task.h>
#include <alchemy/timer.h>
#include <rtdm/gpio.h>

#define LEDBLINKPERIOD 5e8

RT_TASK ledToggle;
RT_TASK ledReadInterrupt;

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
    int xeno_trigger = GPIO_TRIGGER_EDGE_RISING;

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

void led_toggle(void *arg) {
    int led = *(int *)arg;
    int led_state = 0;

    while (1) {
        if (write(led, &led_state, sizeof(led_state)) != sizeof(led_state)) {
            printf("Error while writing to led.\n");
            exit(1);
        }
        led_state = led_state == 0 ? 1 : 0;
        rt_task_sleep(LEDBLINKPERIOD / 2);
    }
}

void led_interrupt(void *arg) {
    int led_read = *(int *)arg;
    int value = 0;
    int counter = 0;

    while (1) {
        if (read(led_read, &value, sizeof(value)) != sizeof(value)) {
            printf("Error while reading led interrupt.\n");
            exit(1);
        }
        ++counter;
        printf("Interrupt recieved, counter: %d\n", counter);
    }
}


int main(int argc, char *argv[]) {
    int led = open_led_gpio();
    int led_read = open_led_read_gpio();
    rt_task_create(&ledToggle, "led_toggle", 0, 50, 0);
    rt_task_create(&ledReadInterrupt, "led_interrupt", 0, 70, 0);
    rt_task_start(&ledToggle, &led_toggle, &led);
    rt_task_start(&ledReadInterrupt, &led_interrupt, &led_read);

    pause();
}
