/*
 * バッテリー残量をステータスLEDの点滅回数(1〜5回)で表示するビヘイビア
 *
 * 81-100%: 5回 / 61-80%: 4回 / 41-60%: 3回 / 21-40%: 2回 / 0-20%: 1回
 */

#define DT_DRV_COMPAT zmk_behavior_battery_led

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>

#include <zmk/battery.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define LED_NODE DT_PHANDLE(DT_INST(0, zmk_behavior_battery_led), led)
static const struct gpio_dt_spec led_gpio = GPIO_DT_SPEC_GET(LED_NODE, gpios);

// 点滅タイミング(キーマップのblink-ms/pause-msプロパティで調整可能)
#define BLINK_ON_MS DT_INST_PROP(0, blink_ms)
#define BLINK_OFF_MS DT_INST_PROP(0, pause_ms)

static struct k_work_delayable blink_work;
static uint8_t remaining_blinks;
static bool led_on;

static void blink_work_handler(struct k_work *work) {
    if (!led_on && remaining_blinks > 0) {
        gpio_pin_set_dt(&led_gpio, 1);
        led_on = true;
        k_work_schedule(&blink_work, K_MSEC(BLINK_ON_MS));
    } else {
        gpio_pin_set_dt(&led_gpio, 0);
        if (led_on) {
            led_on = false;
            remaining_blinks--;
        }
        if (remaining_blinks > 0) {
            k_work_schedule(&blink_work, K_MSEC(BLINK_OFF_MS));
        }
    }
}

static uint8_t battery_level_to_blinks(uint8_t soc) {
    if (soc > 80) {
        return 5;
    } else if (soc > 60) {
        return 4;
    } else if (soc > 40) {
        return 3;
    } else if (soc > 20) {
        return 2;
    }
    return 1;
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    // 表示中の再トリガーは無視する
    if (k_work_delayable_is_pending(&blink_work)) {
        return ZMK_BEHAVIOR_OPAQUE;
    }

    uint8_t soc = zmk_battery_state_of_charge();
    remaining_blinks = battery_level_to_blinks(soc);
    led_on = false;
    LOG_INF("Battery level %d%%, blinking %d times", soc, remaining_blinks);
    k_work_schedule(&blink_work, K_NO_WAIT);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_battery_led_init(const struct device *dev) {
    k_work_init_delayable(&blink_work, blink_work_handler);

    if (!device_is_ready(led_gpio.port)) {
        LOG_ERR("LED GPIO device not ready");
        return -ENODEV;
    }

    return gpio_pin_configure_dt(&led_gpio, GPIO_OUTPUT_INACTIVE);
}

static const struct behavior_driver_api behavior_battery_led_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    // 両手それぞれのLEDが自分側の電池残量を表示する
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

BEHAVIOR_DT_INST_DEFINE(0, behavior_battery_led_init, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_battery_led_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
