#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include "bus/zbus_channels.h"
#include "settings/settings_nvs.h"

LOG_MODULE_REGISTER(sg_shell, LOG_LEVEL_INF);

static int cmd_status(const struct shell *sh, size_t argc, char **argv) {
    enum sg_state st;
    zbus_chan_read(&system_state, &st, K_MSEC(10));
    const char *nomes[] = { "Desarmado", "Armado", "Alarme" };
    shell_print(sh, "Estado atual: %s", nomes[st]);
    shell_print(sh, "Threshold: %u LSB", sg_settings_get_threshold());
    return 0;
}

static int cmd_arm(const struct shell *sh, size_t argc, char **argv) {
    struct sg_arm_cmd cmd = { .arm = true };
    zbus_chan_pub(&arm_event, &cmd, K_MSEC(10));
    shell_print(sh, "Evento de armamento publicado.");
    return 0;
}

static int cmd_disarm(const struct shell *sh, size_t argc, char **argv) {
    struct sg_arm_cmd cmd = { .arm = false };
    zbus_chan_pub(&arm_event, &cmd, K_MSEC(10));
    shell_print(sh, "Evento de desarmamento publicado.");
    return 0;
}

static int cmd_threshold(const struct shell *sh, size_t argc, char **argv) {
    if (argc < 2) {
        shell_print(sh, "Uso: sg threshold <valor>");
        return -EINVAL;
    }

    uint16_t val = (uint16_t)atoi(argv[1]);
    sg_settings_set_threshold(val);
    shell_print(sh, "Threshold definido para %u LSB (salvo em flash).", val);
    return 0;
}

static int cmd_reset(const struct shell *sh, size_t argc, char **argv) {
    struct sg_arm_cmd cmd = { .arm = false };
    zbus_chan_pub(&arm_event, &cmd, K_MSEC(10));
    shell_print(sh, "Alarme reconhecido. Sistema desarmado.");
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sg_cmds,
    SHELL_CMD(status, NULL, "Exibe o estado do threshold", cmd_status),
    SHELL_CMD(arm, NULL, "Arma o sistema", cmd_arm),
    SHELL_CMD(disarm, NULL, "Desarma o sistema", cmd_disarm),
    SHELL_CMD(threshold, NULL, "Define o threshold em LSB", cmd_threshold),
    SHELL_CMD(reset, NULL, "Reconhece o alarme", cmd_reset),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(sg, &sg_cmds, "SafeGuard Node", NULL);