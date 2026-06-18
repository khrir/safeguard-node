#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(safeguard, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("SafeGuard Node iniciando...");

	while (1) {
		LOG_INF("rodando");
		k_sleep(K_SECONDS(100));
	}

	return 0;
}
