#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "bus/zbus_channels.h"

LOG_MODULE_REGISTER(sg_buzzer, LOG_LEVEL_INF);

void sg_buzzer_thread(void *a, void *b, void *c) {
    const struct zbus_channel *channel;
    enum sg_state st;

    while (!zbus_sub_wait(&buzzer_sub, &channel, K_FOREVER)) {
        zbus_chan_read(&system_state, &st, K_MSEC(10));

        switch (st) {
            case SG_DISARMED:
                LOG_INF("Buzzer: off");
                break;
            case SG_ARMED:
                LOG_INF("Buzzer: on continuo");
                break;
            case SG_ALARM:
                LOG_INF("Buzzer: on casual");
                break;
        }
    }
}

K_THREAD_DEFINE(buzzer_tid, 1024, sg_buzzer_thread, NULL, NULL, NULL, 6, 0, 0);