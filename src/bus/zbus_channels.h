#pragma once
#include <zephyr/zbus/zbus.h>

struct sg_accel { int16_t x, y, z; };
struct sg_arm_cmd { bool arm; };

enum sg_state { SG_DISARMED = 0, SG_ARMED, SG_ALARM };

// canais
ZBUS_CHAN_DECLARE(accel_data);
ZBUS_CHAN_DECLARE(system_state);
ZBUS_CHAN_DECLARE(arm_event);

// subscribers — threads que vão ouvir cada canal
ZBUS_OBS_DECLARE(proc_sub, led_sub, buzzer_sub);