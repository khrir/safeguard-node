#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "bus/zbus_channels.h"

LOG_MODULE_REGISTER(sg_led, LOG_LEVEL_INF);

void sg_led_thread(void *a, void *b, void *c) {
    const struct zbus_channel *channel;
    enum sg_state st;

    while (!zbus_sub_wait(&led_sub, &channel, K_FOREVER)) {
        zbus_chan_read(&system_state, &st, K_MSEC(10));

        switch (st) {
            case SG_DISARMED:
                LOG_INF("LED: apagado");
                break;
            case SG_ARMED:
                LOG_INF("LED: aceso fixo");
                break;
            case SG_ALARM:
                LOG_INF("LED: piscando 5 Hz");
                break;
        }
    }
}

K_THREAD_DEFINE(led_tid, 1024, sg_led_thread, NULL, NULL, NULL, 6, 0, 0);