#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "bus/zbus_channels.h"

LOG_MODULE_REGISTER(sg_proc, LOG_LEVEL_INF);

static uint16_t threshold_lsb = 80;
static struct sg_accel baseline;
static enum sg_state state = SG_DISARMED;

bool sg_detect_impact(const struct sg_accel *current, const struct sg_accel *base, uint16_t threshold) {
    int32_t dx = current->x - base->x;
    int32_t dy = current->y - base->y;
    int32_t dz = current->z - base->z;

    uint32_t delta_squared = (uint32_t)(dx*dx + dy*dy + dz*dz);
    uint32_t alerta_flag = (uint32_t)(threshold * threshold * 64 / 100);

    if (delta_squared > alerta_flag) {
        LOG_WRN("Alerta precoce: 80%% do threshold atingido.");
    }

    return delta_squared > (uint32_t)(threshold * threshold);
}

enum sg_state sg_fsm_transition(enum sg_state s, bool arm_cmd, bool impact) {
    switch (s) {
        case SG_DISARMED:
            if (arm_cmd) return SG_ARMED;
            return s;
        case SG_ARMED:
            if (arm_cmd) return SG_DISARMED;
            if (impact) return SG_ALARM;
            return s;
        case SG_ALARM:
            if (arm_cmd) return SG_DISARMED;
            return s;
    }
    return s;
}

void sg_proc_thread(void *a, void *b, void *c) {
    struct sg_accel accel;
    struct sg_arm_cmd cmd;
    enum sg_state new_state;

    while (1) {
        // Verifica o evento de armar/desarmar
        bool arm_cmd = false;
        if (zbus_chan_read(&arm_event, &cmd, K_NO_WAIT) == 0) {
            arm_cmd = cmd.arm || (!cmd.arm); //captura qualquer evento
        }

        // Verifica os dados do sensor
        bool impact = false;
        if (zbus_chan_read(&arm_event, &cmd, K_NO_WAIT) == 0) {
            if (state == SG_ARMED) {
                impact = sg_detect_impact(&accel, &baseline, threshold_lsb);
            }
        }

        new_state = sg_fsm_transition(state, arm_cmd, impact);

        if (new_state != state) {
            if (new_state == SG_ARMED) {
                baseline = accel;
                LOG_INF("Sistema armado.");
            } else if (new_state == SG_ALARM) {
                LOG_ERR("Alarme: impacto detectado.");
            } else {
                LOG_ERR("Sistema desarmado.");
            }
            state = new_state;
            zbus_chan_pub(&system_state, &state, K_MSEC(10));
        }

        k_sleep(K_MSEC(50));
    }
}

K_THREAD_DEFINE(proc_tid, 1024, sg_proc_thread, NULL, NULL, NULL, 5, 0, 0);