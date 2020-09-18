#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <alchemy/task.h>
#include <alchemy/timer.h>
#include <rtdm/gpio.h>

#define LEDBLINKPERIOD 5e8

RT_TASK ledToggle;

int open_led_gpio(void) {
    int value = 0;
    int fd = open("/dev/rtdm/pinctrl-bcm2835/gpio23",O_WRONLY);
    if (fd == -1) {
        printf("Could not open LED (gpio23) device file.\n");
        exit(1);
    }
    //set device to output mode with special device request GPIO_RTIOC_DIR_OUT
    if (ioctl(fd, GPIO_RTIOC_DIR_OUT, &value) == -1) {
        printf("Could not do GPIO_RTIOC_DIR_OUT on LED (gpio23) file descriptor.\n");
        exit(1);
    }
    return fd;
}

void close_led_gpio(int fd) {
    if (close(fd) == -1) {
        printf("Could not close LED (gpio23) file descriptor.\n");
        exit(1);
    }
}


int main(int argc, char *argv[]) {
    int led_state = 0;
    int led = open_led_gpio();
    for (int i = 0; i < 20; ++i) {
        if (write(led, &led_state, sizeof(led_state)) != sizeof(led_state)) {
            printf("Error while writing to led.\n");
            exit(1);
        }
        led_state = led_state == 0 ? 1 : 0;
        rt_timer_spin(LEDBLINKPERIOD / 2);
    }
    close_led_gpio(led);
}
