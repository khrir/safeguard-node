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
ZBUS_SUBSCRIBER_DECLARE(proc_sub);    /* proc_thread ouve: accel_data + arm_event */
ZBUS_SUBSCRIBER_DECLARE(led_sub);     /* led_thread ouve: system_state */
ZBUS_SUBSCRIBER_DECLARE(buzzer_sub);  /* buzzer_thread ouve: system_state */