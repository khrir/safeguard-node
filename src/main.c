#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "settings/settings_nvs.h"

LOG_MODULE_REGISTER(safeguard, LOG_LEVEL_INF);

int main(void)
{
	sg_settings_init();
	LOG_INF("SafeGuard Node iniciando...");

	while (1) {
		k_sleep(K_SECONDS(100));
	}

	return 0;
}
