#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_REGISTER(sg_settings, LOG_LEVEL_INF);

static uint16_t threshold_lsb = 80;

uint16_t sg_settings_get_threshold(void) {
    return threshold_lsb;
}

void sg_settings_set_threshold(uint16_t valor) {
    threshold_lsb = valor;
    settings_save_one("sg/threshold", &threshold_lsb, sizeof(threshold_lsb));
}

static int sg_settings_load_cb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg, void *param) {
    if (strcmp(key, "threshold") == 0) {
        read_cb(cb_arg, &threshold_lsb, sizeof(threshold_lsb));
    }

    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(sg, "sg", NULL, sg_settings_load_cb, NULL, NULL);

void sg_settings_init(void) {
    settings_subsys_init();
    settings_load();
    LOG_INF("Threshold carregado: %u LSB", threshold_lsb);
}