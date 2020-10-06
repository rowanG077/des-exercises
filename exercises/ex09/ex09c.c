#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <alchemy/mutex.h>
#include <alchemy/sem.h>
#include <alchemy/task.h>
#include <rtdm/gpio.h>

/* Amount of LEDs in the column */
#define LEDCOUNT 8
/* The pixel width of a character */
#define CHARWIDTH 8

/* How many tenth of degrees need to pass to determine a full pixel. */
static const uint16_t TENTHDEG_PER_PIXEL = 50;
/* Characters are drawn starting at a target angle. */
static const uint16_t DRAW_TARGET = 1800;
/* Representation of characters */
// clang-format off
static const uint8_t LETTER[CHARWIDTH] =
{
	0b00000000,
	0b11000011,
	0b01100110,
	0b00111100,
	0b01100110,
	0b11000011,
	0b00000000
};
// clang-format on

/* Semaphore to ensure proper execution. */
static RT_SEM measure_sync;

/* Mutex to guard global period data. */
static RT_MUTEX measure_lock;

/* Amount of nanoseconds per tenth degree in the currently measured period */
static RTIME nanos_per_tenth_deg = 0 - 1;
/* Time that the sensor last triggered. */
static RTIME last_sensor_trigger = 0;

/**
 * Close the given file descriptor.
 *
 * \param fd File descriptor.
 */
static void close_gpio(int fd)
{
	if (close(fd) == -1) {
		fprintf(stderr, "Could not close gpio file descriptor.\n");
		exit(1);
	}
}

/**
 * Open the light sensor connected to a GPIO pin.
 *
 * \return File descriptor.
 */
static int open_lightsensor_gpio(void)
{
	int xeno_trigger = GPIO_TRIGGER_EDGE_RISING;

	int fd = open("/dev/rtdm/pinctrl-bcm2835/gpio23", O_RDONLY);
	if (fd == -1) {
		fprintf(stderr,
			"Could not open lightsensor (gpio23) device file.\n");
		exit(1);
	}

	if (ioctl(fd, GPIO_RTIOC_IRQEN, &xeno_trigger) == -1) {
		fprintf(stderr,
			"Could not do GPIO_RTIOC_IRQEN on lightsensor (gpio23) "
			"file descriptor.\n");
		exit(1);
	}

	return fd;
}

/**
 * Open an LED connected to a GPIO pin.
 *
 * \param path LED file to open.
 * \return File descriptor.
 */
static int open_led_gpio(const char *const path)
{
	int value = 0;
	int fd = open(path, O_WRONLY);
	if (fd == -1) {
		fprintf(stderr, "Could not open LED(%s) device file.\n", path);
		exit(1);
	}

	// set device to output mode with special device request
	// GPIO_RTIOC_DIR_OUT
	if (ioctl(fd, GPIO_RTIOC_DIR_OUT, &value) == -1) {
		fprintf(stderr,
			"Could not do GPIO_RTIOC_DIR_OUT on LED(%s) file "
			"descriptor.\n",
			path);
		exit(1);
	}
	return fd;
}

/**
 * Set the state of the given LED.
 *
 * \param led LED file descriptor.
 * \param curr_state Current LED state.
 * \param new_state New LED state.
 */
static void set_led(int led, uint32_t curr_state, uint32_t new_state)
{
	// Do not do costly IO if not necessary
	if (curr_state == new_state) {
		return;
	}
	if (write(led, &new_state, sizeof(new_state)) != sizeof(new_state)) {
		fprintf(stderr, "Error while writing to led.\n");
		exit(1);
	}
}

/**
 * Estimate the current position of the LEDs on the disk.
 *
 * \return Current degrees of the LED column if it can be determined, -1
 * otherwise.
 */
static int16_t estimate_disk_pos()
{
	RTIME nanos_per_tenth_deg_;
	RTIME last_sensor_trigger_;
	RTIME delta;
	int16_t deg_read_pos;

	rt_mutex_acquire(&measure_lock, TM_INFINITE);
	nanos_per_tenth_deg_ = nanos_per_tenth_deg;
	last_sensor_trigger_ = last_sensor_trigger;
	rt_mutex_release(&measure_lock);

	delta = rt_timer_read() - last_sensor_trigger_;

	/*
	 * Disk position can only be estimated if we've gotten an update
	 * recently.
	 */
	deg_read_pos = delta / nanos_per_tenth_deg_;
	if (deg_read_pos > 2 * 3600) {
		return -1;
	}

	return deg_read_pos % 3600;
}

/**
 * Set the state for the entire LED column.
 *
 * \param leds Array of file descriptors for each LED in the column.
 * \param curr_states Current state of each LED in the column,
 * \param column New state for each LED.
 */
static void write_column(
	int leds[LEDCOUNT], uint32_t curr_states[LEDCOUNT], uint8_t column)
{
	for (uint8_t i = 0; i < LEDCOUNT; ++i) {
		uint8_t state = (column >> i) & 1;
		set_led(leds[i], curr_states[i], state);
		curr_states[i] = state;
	}
}

/**
 * The LED controller task draws the character at the correct position.
 */
static void led_controller(void *arg)
{
	int16_t last_deg;
	int16_t current_deg;
	int leds[LEDCOUNT];
	uint32_t led_states[LEDCOUNT] = {0};
	int16_t write_pos[CHARWIDTH];
	int16_t start_pos;
	int16_t pos;

	leds[0] = open_led_gpio("/dev/rtdm/pinctrl-bcm2835/gpio2");
	leds[1] = open_led_gpio("/dev/rtdm/pinctrl-bcm2835/gpio3");
	leds[2] = open_led_gpio("/dev/rtdm/pinctrl-bcm2835/gpio4");
	leds[3] = open_led_gpio("/dev/rtdm/pinctrl-bcm2835/gpio17");
	leds[4] = open_led_gpio("/dev/rtdm/pinctrl-bcm2835/gpio27");
	leds[5] = open_led_gpio("/dev/rtdm/pinctrl-bcm2835/gpio22");
	leds[6] = open_led_gpio("/dev/rtdm/pinctrl-bcm2835/gpio10");
	leds[7] = open_led_gpio("/dev/rtdm/pinctrl-bcm2835/gpio9");

	/*
	 * Determine the position to draw at. If a character needs to be drawn
	 * at position x then we need to draw some degrees before x and some
	 * degrees after x.
	 */
	start_pos = (TENTHDEG_PER_PIXEL / 2)
		+ (((CHARWIDTH - 1) / 2) * TENTHDEG_PER_PIXEL);
	if (start_pos > DRAW_TARGET) {
		start_pos = 3600 - start_pos;
	} else {
		start_pos = DRAW_TARGET - start_pos;
	}

	for (size_t i = 0; i < CHARWIDTH; ++i) {
		write_pos[i] = (start_pos + i * TENTHDEG_PER_PIXEL) % 3600;
	}

	while (1) {
		current_deg = estimate_disk_pos();

		/* Turn the LEDS off if we can't determine the position */
		if (current_deg == -1) {
			write_column(leds, led_states, 0);
			continue;
		}

		for (size_t i = 0; i < CHARWIDTH; ++i) {
			if (last_deg < write_pos[i]
				&& current_deg >= write_pos[i]) {
				write_column(leds, led_states, LETTER[i]);
				break;
			}
		}

		last_deg = current_deg;

		rt_task_sleep(200e3);
	}

	for (size_t i = 0; i < LEDCOUNT; ++i) {
		close_gpio(leds[i]);
	}

	return;
}

/**
 * The disk measure task listens to interrupts from the light sensor and
 * calculates the information related to the rotation of the disk.
 */
static void disk_measure(void *arg)
{
	int lightsensor = open_lightsensor_gpio();
	int lightsensor_value = 0;
	RTIME disk_period = 0;
	RTIME current_t = rt_timer_read();
	RTIME last_trigger_t = current_t;
	RTIME nanos_per_tenth_deg_ = 0 - 1; // force underflow

	while (1) {
		if (read(lightsensor, &lightsensor_value,
			    sizeof(lightsensor_value))
			!= sizeof(lightsensor_value)) {
			fprintf(stderr,
				"Error while reading from light sensor.\n");
			exit(1);
		}

		current_t = rt_timer_read();
		disk_period = current_t - last_trigger_t;

		nanos_per_tenth_deg_ = disk_period / 3600;

		rt_mutex_acquire(&measure_lock, TM_INFINITE);
		nanos_per_tenth_deg = nanos_per_tenth_deg_;
		last_sensor_trigger = current_t;
		rt_mutex_release(&measure_lock);

		last_trigger_t = current_t;
	}

	close_gpio(lightsensor);

	rt_sem_v(&measure_sync);
	return;
}

int main(int argc, char *argv[])
{
	RT_TASK controller_task;
	RT_TASK measure_task;

	rt_sem_create(&measure_sync, "measure_sync", 0, S_FIFO);
	rt_mutex_create(&measure_lock, "measure_lock");
	rt_task_create(&measure_task, "measure_task", 0, 70, 0);
	rt_task_create(&controller_task, "controller_task", 0, 50, 0);

	rt_task_start(&measure_task, &disk_measure, NULL);
	rt_task_start(&controller_task, &led_controller, NULL);

	rt_sem_p(&measure_sync, TM_INFINITE);
	return 0;
}
