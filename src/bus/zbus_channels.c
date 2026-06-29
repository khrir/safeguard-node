#include "zbus_channels.h"

ZBUS_SUBSCRIBER_DEFINE(proc_sub,   8);
ZBUS_SUBSCRIBER_DEFINE(led_sub,    4);
ZBUS_SUBSCRIBER_DEFINE(buzzer_sub, 4);

ZBUS_CHAN_DEFINE(accel_data, struct sg_accel, NULL, NULL, ZBUS_OBSERVERS(proc_sub), ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(system_state, enum sg_state, NULL, NULL, ZBUS_OBSERVERS(led_sub, buzzer_sub), ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(arm_event, struct sg_arm_cmd, NULL, NULL, ZBUS_OBSERVERS(proc_sub), ZBUS_MSG_INIT(0));