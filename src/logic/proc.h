#pragma once
#include "bus/zbus_channels.h"

bool sg_detect_impact(const struct sg_accel *current, const struct sg_accel *base, uint16_t threshold);

enum sg_state sg_fsm_transition(enum sg_state s, bool arm_cmd, bool impact);