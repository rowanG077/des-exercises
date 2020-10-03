#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>

#include <alchemy/sem.h>
#include <alchemy/task.h>
#include <alchemy/mutex.h>
#include <rtdm/gpio.h>

#define LEDCOUNT 8
#define CHARWIDTH 8

// How many tenth of degrees need to pass to determine a full pixel.
static const uint16_t TENTHDEGPERPIXEL = 50;
// At what offset from the light sensor the char will be drawn.
static const uint16_t DRAWTARGET = 1800;
static const uint8_t letter[CHARWIDTH] = {
    0b00000000,
    0b11000011,
    0b01100110,
    0b00111100,
    0b01100110,
    0b11000011,
    0b00000000
};

RT_MUTEX measureLock;
RTIME nanosPerTenthDeg = 0;
RTIME lastSensorTrigger = 0;

void close_gpio(int fd) {
    if (close(fd) == -1) {
        fprintf(stderr, "Could not close gpio file descriptor.\n");
        exit(1);
    }
}

int open_lightsensor_gpio(void) {
    int xeno_trigger = GPIO_TRIGGER_EDGE_RISING;

    int fd = open("/dev/rtdm/pinctrl-bcm2835/gpio23", O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Could not open lightsensor (gpio23) device file.\n");
        exit(1);
    }
    if (ioctl(fd, GPIO_RTIOC_IRQEN, &xeno_trigger) == -1) {
        fprintf(stderr, "Could not do GPIO_RTIOC_IRQEN on lightsensor (gpio23) file descriptor.\n");
        exit(1);
    }
    return fd;
}

int open_led_gpio(const char* const path) {
    int value = 0;
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        fprintf(stderr, "Could not open LED(%s) device file.\n", path);
        exit(1);
    }

    // set device to output mode with special device request GPIO_RTIOC_DIR_OUT
    if (ioctl(fd, GPIO_RTIOC_DIR_OUT, &value) == -1) {
        fprintf(stderr, "Could not do GPIO_RTIOC_DIR_OUT on LED(%s) file descriptor.\n", path);
        exit(1);
    }
    return fd;
}

void set_led(int led, uint32_t curr_state, uint32_t new_state) {
    // Do not do costly IO if not necessary
    if (curr_state == new_state) {
        return;
    }
    if (write(led, &new_state, sizeof(new_state)) != sizeof(new_state)) {
        fprintf(stderr, "Error while writing to led.\n");
        exit(1);
    }
}

uint16_t estimate_disk_pos() {
    RTIME nanosPerTenthDeg_;
    RTIME lastSensorTrigger_;
    RTIME delta;

    rt_mutex_acquire(&measureLock, TM_INFINITE);
    nanosPerTenthDeg_ = nanosPerTenthDeg;
    lastSensorTrigger_ = lastSensorTrigger;
    rt_mutex_release(&measureLock);

    delta = rt_timer_read() - lastSensorTrigger_;

    return (delta / nanosPerTenthDeg_) % 3600;
}

void write_column(int leds[LEDCOUNT], uint32_t curr_states[LEDCOUNT], uint8_t column) {
    for (uint8_t i = 0; i < LEDCOUNT; ++i) {
        uint8_t state = (column >> i) & 1;
        set_led(leds[i], curr_states[i], state);
        curr_states[i] = state;
    }
}

void led_controller(void* arg) {
    uint16_t last_deg;
    uint16_t current_deg;
    int leds[LEDCOUNT];
    uint32_t ledStates[LEDCOUNT] = { 0 };
    uint16_t writePositions[CHARWIDTH];

    leds[0] = open_led_gpio("/dev/rtdm/pinctrl-bcm2835/gpio2");
    leds[1] = open_led_gpio("/dev/rtdm/pinctrl-bcm2835/gpio3");
    leds[2] = open_led_gpio("/dev/rtdm/pinctrl-bcm2835/gpio4");
    leds[3] = open_led_gpio("/dev/rtdm/pinctrl-bcm2835/gpio17");
    leds[4] = open_led_gpio("/dev/rtdm/pinctrl-bcm2835/gpio27");
    leds[5] = open_led_gpio("/dev/rtdm/pinctrl-bcm2835/gpio22");
    leds[6] = open_led_gpio("/dev/rtdm/pinctrl-bcm2835/gpio10");
    leds[7] = open_led_gpio("/dev/rtdm/pinctrl-bcm2835/gpio9");

    for (size_t i = 0; i < CHARWIDTH; ++i) {
        writePositions[i] = DRAWTARGET - (TENTHDEGPERPIXEL / 2) - (((CHARWIDTH - 1) / 2) * TENTHDEGPERPIXEL) + i * TENTHDEGPERPIXEL;
    }

    while (1) {
        current_deg = estimate_disk_pos();

        for (size_t i = 0; i < CHARWIDTH; ++i) {
            if (last_deg < writePositions[i] && current_deg > writePositions[i]) {
                write_column(leds, ledStates, letter[i]);
                break;
            }
        }

        last_deg = current_deg;
    }

    for (size_t i = 0; i < LEDCOUNT; ++i) {
        close_gpio(leds[i]);
    }

    return;
}

void disk_measure(void* arg) {
    int lightsensor = open_lightsensor_gpio();
    int lightsensorValue = 0;
    RTIME diskPeriod = 0;
    RTIME currentT = rt_timer_read();
    RTIME lastTriggerT = currentT;
    RTIME nanosPerTenthDeg_ = 0 - 1; // force underflow

    while (1) {
        if (read(lightsensor, &lightsensorValue, sizeof(lightsensorValue)) != sizeof(lightsensorValue)) {
            printf("Error while reading from light sensor.\n");
            exit(1);
        }
        currentT = rt_timer_read();
        diskPeriod = lastTriggerT - currentT;

        nanosPerTenthDeg_ = diskPeriod / 3600;

        rt_mutex_acquire(&measureLock, TM_INFINITE);
        nanosPerTenthDeg = nanosPerTenthDeg_;
        lastSensorTrigger = currentT;
        rt_mutex_release(&measureLock);

        lastTriggerT = currentT;
    }

    close_gpio(lightsensor);

    return;
}

int main(int argc, char* argv[])
{
    RT_TASK controllerTask;
    RT_TASK measureTask;

    rt_mutex_create(&measureLock, "measureLock");
    rt_task_create(&measureTask, "measureTask", 0, 70, 0);
    rt_task_create(&controllerTask, "controllerTask", 0, 50, 0);

    rt_task_start(&measureTask, &disk_measure, NULL);
    rt_task_start(&controllerTask, &led_controller, NULL);

    pause();

    return 0;
}
