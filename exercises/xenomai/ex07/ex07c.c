#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <alchemy/task.h>
#include <alchemy/timer.h>
#include <rtdm/gpio.h>

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

int open_btn_gpio(void) {
    int xeno_trigger = GPIO_TRIGGER_EDGE_FALLING | GPIO_TRIGGER_EDGE_RISING;

    int fd = open("/dev/rtdm/pinctrl-bcm2835/gpio23", O_RDONLY);
    if (fd == -1) {
        printf("Could not open BUTTON (gpio23) device file.\n");
        exit(1);
    }
    if (ioctl(fd, GPIO_RTIOC_IRQEN, &xeno_trigger) == -1) {
        printf("Could not do GPIO_RTIOC_IRQEN on BUTTON (gpio23) file descriptor.\n");
        exit(1);
    }
    return fd;
}

int main(int argc, char *argv[]) {
    RTIME debounce = 50e6;
    RTIME lastBtnPress = 0;

    int led_state = 1;
    int last_btn_state = 1;
    int btn_state = 0;

    int led = open_led_gpio();
    int btn = open_btn_gpio();

    int counter = 0;

    while (1) {
        if (read(btn, &btn_state, sizeof(btn_state)) != sizeof(btn_state)) {
            printf("Error while reading button interrupt.\n");
            exit(1);
        }
        RTIME now = rt_timer_read();

        if (lastBtnPress + debounce > now || last_btn_state == btn_state) {
            continue;
        }

        lastBtnPress = now;

        last_btn_state = btn_state;
        if (btn_state != 0) {
            continue;
        }

        ++counter;
        printf("Interrupt recieved, counter: %d\n", counter);

        if (write(led, &led_state, sizeof(led_state)) != sizeof(led_state)) {
            printf("Error while writing to led.\n");
            exit(1);
        }

        led_state = led_state == 0 ? 1 : 0;
    }
}
