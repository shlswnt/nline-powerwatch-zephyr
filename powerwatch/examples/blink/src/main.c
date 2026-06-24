#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(Blink, LOG_LEVEL_INF);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
	LOG_INF("Blink booting on %s", CONFIG_BOARD);

	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("LED not ready");
		return 0;
	}
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

	int i = 0;

	while (1) {
		gpio_pin_toggle_dt(&led);

		i++;
		LOG_INF("%d", i);
		
		k_msleep(1000);
	}
	return 0;
}
