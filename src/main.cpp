#include "Display.hpp"
#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define DISPLAY0_NODE DT_ALIAS(display0)

LOG_MODULE_REGISTER(main);

#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

#if !DT_NODE_HAS_STATUS(DISPLAY0_NODE, okay)
#error "Unsupported board: zephyr_display devicetree alias is not defined"
#endif

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct device *displayDevice = DEVICE_DT_GET(DISPLAY0_NODE);

int main(void) {

  Display display(displayDevice);

  display.Init();

  while (1) {
    k_msleep(display.Update());
  }

  return 0;
}

void blink(const struct gpio_dt_spec *led, uint32_t sleep_ms) {
  int cnt = 0;
  int ret;

  if (!device_is_ready(led->port)) {
    printk("Error: %s device is not ready\n", led->port->name);
    return;
  }

  ret = gpio_pin_configure_dt(led, GPIO_OUTPUT);
  if (ret != 0) {
    printk("Error %d: failed to configure pin %d (LED0)\n", ret, led->pin);
    return;
  }

  while (1) {
    gpio_pin_set(led->port, led->pin, cnt % 2);

    // struct printk_data_t tx_data = {.led = id, .cnt = cnt};

    // size_t size = sizeof(struct printk_data_t);
    // char *mem_ptr = k_malloc(size);
    // __ASSERT_NO_MSG(mem_ptr != 0);

    // memcpy(mem_ptr, &tx_data, size);

    // k_fifo_put(&printk_fifo, mem_ptr);

    k_msleep(sleep_ms);
    cnt++;
  }
}

void blink0(void) { blink(&led0, 1000); }

K_THREAD_DEFINE(blink0_id, 1024, blink0, NULL, NULL, NULL, 7, 0, 0);
// K_THREAD_DEFINE(uart_out_id, STACKSIZE, uart_out, NULL, NULL, NULL, PRIORITY,
// 0,
//                 0);