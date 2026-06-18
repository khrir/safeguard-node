# SafeGuard Node — Roteiro de Apresentação e Guia de Implementação

> **Apresentação da proposta:** 19/06/2026 às 11h10 (após a prova)  
> **Entrega final:** 26/06/2026  
> **Referência para uso durante a semana de implementação.**

---

## Parte 1 — Roteiro da Apresentação (≈ 8–10 min)

### Abertura (1 min)

**Fale assim:**

> "O projeto se chama SafeGuard Node — um nó de monitoramento de segurança física.
> A ideia é simples: imagine um rack de servidores ou um cofre de transporte.
> Você quer saber se alguém mexeu nele enquanto você não estava olhando.
> O dispositivo usa o ADXL345, que já tenho funcionando com driver SPI bare-metal,
> para detectar qualquer impacto ou movimento quando o sistema está armado."

**Por que isso funciona como abertura:** você ancora o projeto num problema real e imediato antes de falar de tecnologia. Qualquer pessoa na bancada entende o que é um rack sendo movido sem autorização.

---

### Bloco 1 — A máquina de estados (2 min)

Mostre o diagrama da FSM (slide 2 do PDF).

**Script:**

> "O sistema tem três estados operacionais.
> Desarmado: LED apagado, sistema apenas monitorando.
> Armado: LED aceso fixo, baseline de aceleração capturado.
> Alarme: LED piscando a 5 Hz, evento registrado em log.
>
> A transição entre estados é feita por pressão longa no botão físico — mais de 1,5 segundos,
> para evitar falso gatilho. E também por linha de comando via shell UART.
>
> Quando o sistema arma, ele captura a leitura XYZ atual como referência.
> A cada 50 ms, compara a leitura nova com esse baseline.
> Se a diferença — a delta magnitude — ultrapassar o threshold configurado, vai para alarme."

---

### Bloco 2 — Requisitos de hardware (1,5 min)

| Item | O que dizer |
|------|-------------|
| UART de debug | "UART2 via ST-LINK, log de cada transição de estado e shell interativo na mesma porta." |
| LED | "LD2 onboard — três estados da FSM mapeados visualmente sem abrir terminal." |
| Botão | "USER button onboard, ISR com timer de software para detectar pressão longa." |
| Sensor SPI | "ADXL345 sem CONFIG_ADXL345 — protocolo montado byte a byte, já validado no hardware." |

**Antecipe a pergunta sobre a opção sensor:**
> "Escolhi a opção sensor porque o driver SPI bare-metal já está implementado e validado.
> Isso me deixa a semana inteira para o que importa: a arquitetura de software."

---

### Bloco 3 — Arquitetura ZBus (2,5 min)

Mostre o diagrama de arquitetura (slide 3 do PDF).

**Script:**

> "A arquitetura usa o ZBus como barramento de eventos com três canais.
>
> sensor_thread lê o ADXL345 a cada 50 ms e publica raw XYZ em accel_data.
> Ela não sabe se o sistema está armado — só coleta dados.
>
> proc_thread é o único guardião da FSM. Ela consome accel_data,
> aplica a lógica de detecção, e publica o estado em system_state.
> Ela também escuta arm_event, que vem tanto do botão quanto do shell.
>
> led_thread consome system_state e atualiza o LED. Não sabe de sensores, não sabe de botão.
>
> Isso é o padrão publish-subscribe que justifica usar um RTOS.
> Se eu quiser adicionar um buzzer ou uma notificação BLE depois,
> é um novo subscriber no system_state — zero mudança nas threads existentes."

---

### Bloco 4 — Requisitos de software (1,5 min)

- **Shell:** "sem shell, alterar o threshold exige recompilar. Com o shell, o operador ajusta a sensibilidade em campo."
- **Settings:** "sem settings, o threshold configurado se perde no reboot. Com NVS, persiste."
- **Kconfig vs Settings:** "Kconfig define o padrão de fábrica. Settings guarda o estado do usuário."
- **ZTEST:** "testo sg_detect_impact() e a FSM como funções puras — sem precisar do hardware, em QEMU."

---

### Encerramento (1 min)

> "O projeto é simples em hardware — um sensor, um LED, um botão.
> Toda a complexidade está na arquitetura de software.
> Cada subsistema do Zephyr tem uma razão funcional justificada:
> o ZBus desacopla as threads, o Settings mantém a configuração entre reboots,
> o Shell dá controle sem recompilação, os testes validam a lógica crítica sem hardware."

**Última frase:**
> "A demo final vai ser ao vivo e em 30 segundos:
> armo o dispositivo, bato na mesa, o LED dispara alarme.
> Depois mostro o shell, altero o threshold, rebooto, e confirmo que persistiu."

---

### Perguntas prováveis e respostas

**"Por que não usar Bluetooth como o requisito 1.4?"**
> "A regra diz 'ao menos uma das interfaces ou um sensor SPI/I2C sem driver pronto'.
> Escolhi o sensor porque já tenho o protocolo implementado e validado.
> BLE adicionaria uma semana de debug de stack de rede sem agregar nada novo
> em termos de arquitetura RTOS — e reduziria a qualidade do que já está feito."

**"A detecção por delta magnitude não dá falsos positivos?"**
> "O threshold é configurável pelo usuário via shell e persistido em NVS.
> O padrão de fábrica é 80 LSB — cerca de 0.3g — que ignora vibração ambiente
> mas detecta qualquer impacto físico intencional no gabinete."

**"Como você valida que o alarme foi correto?"**
> "LOG_ERR é emitido com timestamp do evento. safeguard status mostra
> o contador de eventos desde o último arme. O operador consegue auditar o histórico."

---

## Parte 2 — Estratégia de Implementação (19–26/06)

### O que já existe

```
✅ CMakeLists.txt      — find_package(Zephyr) + target_sources(app PRIVATE src/main.c)
✅ prj.conf            — CONFIG_LOG=y, LOG_DEFAULT_LEVEL=3
✅ src/main.c          — loop mínimo: LOG_INF a cada tick, k_sleep; build e emulação validados
```

O ponto de partida é `main.c` crescendo módulo a módulo — cada adição compila e roda antes da próxima.

**Board de emulação (sem hardware):** `mps2/an385` via QEMU — `native_sim` não funciona no macOS.

```bash
west build -b mps2/an385 safeguard-node --build-dir safeguard-node/build
west build --build-dir safeguard-node/build -t run   # sair: Ctrl+A → X
```

---

### Estrutura de arquivos alvo

```
src/
  main.c              — boot: settings_load, k_thread_create, log inicial
  sensor.c / .h       — sg_sensor_thread(): lê XYZ, publica em accel_data
  proc.c / .h         — sg_proc_thread(): FSM, sg_detect_impact(), baseline
  led.c / .h          — sg_led_thread(): consome system_state, padrão de pisca
  button.c / .h       — ISR + timer long press → publica arm_event
  shell_cmd.c         — SHELL_CMD_REGISTER, subcomandos safeguard *
  settings_nvs.c      — sg_settings_load() / sg_settings_save()
  zbus_channels.c     — ZBUS_CHAN_DEFINE dos 3 canais
tests/
  src/test_detect.c   — unitários: sg_detect_impact(), FSM transitions
  src/test_settings.c — integração: settings round-trip
  CMakeLists.txt
  prj.conf            — CONFIG_ZTEST=y, sem CONFIG_SPI
Kconfig.safeguard     — opções CONFIG_SG_*
```

---

### Git — branches por feature

```bash
git init   # se ainda não for um repositório
git checkout -b feat/refactor-modules   # dia 19: modularizar main.c
git checkout -b feat/zbus-fsm           # dia 20: canais + FSM + LED + botão
git checkout -b feat/impact-detection   # dia 21: sg_detect_impact() + alarme
git checkout -b feat/shell-settings     # dia 22: shell + NVS
git checkout -b feat/tests              # dia 23–24: ZTEST + Twister
git checkout -b feat/polish             # dia 25: README + limpeza final
```

Commits semânticos: `feat:`, `fix:`, `test:`, `docs:`, `refactor:`.

---

### Plano dia a dia

#### Dia 1 — 19/06 (após a apresentação)
**Objetivo:** estrutura modular compilando, comportamento idêntico ao original.

- [ ] `zbus_channels.c` — definir os 3 canais com seus tipos de mensagem
- [ ] Mover `adxl345_read_xyz()` e helpers para `sensor.c`
- [ ] Criar `sg_sensor_thread()` publicando em `accel_data`
- [ ] `main.c` vira boot: inicializa settings, cria threads, loga "SafeGuard Node iniciado"
- [ ] `west build -b nucleo_f401re safeguard-node --build-dir safeguard-node/build -p always` compila sem erros

**Verificação (emulação):** `west build --build-dir safeguard-node/build -t run` → XYZ aparece no log.

---

#### Dia 2 — 20/06
**Objetivo:** FSM central + LED + botão com long press.

- [ ] `proc.c` — enum `sg_state { SG_DISARMED, SG_ARMED, SG_ALARM }` + `sg_proc_thread()` que consome `accel_data` (ainda sem detecção, só FSM com estado hardcoded)
- [ ] `led.c` — `sg_led_thread()` subscreve `system_state`, três padrões com `k_sleep`
- [ ] `button.c` — `GPIO_INT_EDGE_FALLING` em PC13, timer de software de 1,5s, publica `arm_event`
- [ ] `prj.conf`: adicionar `CONFIG_ZBUS=y`

**Verificação:** botão longo → log "sg: armed". LED acende. Botão longo de novo → "sg: disarmed". LED apaga.

---

#### Dia 3 — 21/06
**Objetivo:** detecção de impacto funcionando end-to-end.

- [ ] `sg_detect_impact()` — delta magnitude² sem ponto flutuante:

```c
/* em proc.c — só inteiros */
static bool sg_detect_impact(const struct sg_accel *cur,
                              const struct sg_accel *baseline,
                              uint16_t threshold_lsb)
{
    int32_t dx = cur->x - baseline->x;
    int32_t dy = cur->y - baseline->y;
    int32_t dz = cur->z - baseline->z;
    uint32_t delta_sq = (uint32_t)(dx*dx + dy*dy + dz*dz);
    return delta_sq > (uint32_t)(threshold_lsb * threshold_lsb);
}
```

- [ ] Ao entrar em `SG_ARMED`, capturar baseline
- [ ] A cada `accel_data` recebido em estado ARMED: chamar `sg_detect_impact()` → se true, transitar para `SG_ALARM`, `LOG_ERR("sg: violação detectada")`
- [ ] LOG_WRN quando delta² > 80% do threshold² (alerta precoce)

**Verificação:** armar → bater na mesa → LED pisca, LOG_ERR aparece.

---

#### Dia 4 — 22/06
**Objetivo:** shell + settings NVS end-to-end.

**Shell (`shell_cmd.c`):**
```c
SHELL_CMD_REGISTER(safeguard, &sg_cmds, "SafeGuard Node", NULL);
/* subcomandos: status, arm, disarm, threshold, reset */
```

- [ ] `safeguard status` → estado atual, event_count, threshold_lsb atual
- [ ] `safeguard arm` / `safeguard disarm` → publica em `arm_event` (mesmo que o botão)
- [ ] `safeguard threshold <n>` → valida (10–1000 LSB), salva via settings, atualiza proc_thread
- [ ] `safeguard reset` → zera event_count, restaura threshold padrão do Kconfig, apaga NVS

**Settings (`settings_nvs.c`):**
```c
settings_subsys_init();
settings_load();  /* no boot, em main.c */

/* ao salvar threshold: */
settings_save_one("sg/threshold", &threshold_lsb, sizeof(threshold_lsb));
```

- [ ] `prj.conf`: `CONFIG_SETTINGS=y`, `CONFIG_SETTINGS_NVS=y`, `CONFIG_NVS=y`, `CONFIG_FLASH=y`, `CONFIG_FLASH_MAP=y`

**Verificação:** `safeguard threshold 200` → reboot → `safeguard status` mostra 200.

---

#### Dias 5 e 6 — 23–24/06
**Objetivo:** suite ZTEST passando via Twister.

```c
/* tests/src/test_detect.c */
ZTEST(sg_suite, test_no_impact_below_threshold) {
    struct sg_accel base = {100, 200, 300};
    struct sg_accel cur  = {110, 210, 310}; /* delta = ~17 LSB */
    zassert_false(sg_detect_impact(&cur, &base, 80), NULL);
}

ZTEST(sg_suite, test_impact_above_threshold) {
    struct sg_accel base = {100, 200, 300};
    struct sg_accel cur  = {160, 260, 360}; /* delta = ~104 LSB */
    zassert_true(sg_detect_impact(&cur, &base, 80), NULL);
}

ZTEST(sg_suite, test_fsm_arm_transition) {
    enum sg_state s = SG_DISARMED;
    s = sg_fsm_transition(s, SG_CMD_ARM);
    zassert_equal(s, SG_ARMED, NULL);
}

ZTEST(sg_suite, test_fsm_alarm_on_impact) {
    enum sg_state s = SG_ARMED;
    s = sg_fsm_transition(s, SG_CMD_IMPACT);
    zassert_equal(s, SG_ALARM, NULL);
}
```

```bash
west twister -T safeguard-node/tests -p qemu_cortex_m3 --inline-logs
```

- [ ] `tests/prj.conf`: `CONFIG_ZTEST=y` (sem `CONFIG_SPI` — testa funções puras)
- [ ] `sg_detect_impact()` e `sg_fsm_transition()` devem estar em header linkável pelos testes

---

#### Dia 7 — 25/06
**Objetivo:** documentação, polish, validação final no hardware.

- [ ] `README.md`: descrição do projeto, tabela de wiring, como buildar, como usar o shell, como rodar os testes, o que cada LED significa
- [ ] Testar fluxo completo: boot → arme → bate na mesa → alarme → shell status → reboot → threshold persistido
- [ ] Ajustar LOG_WRN threshold para não spam em vibração ambiente normal
- [ ] `git tag v1.0.0` na main

---

### Plano B — hierarquia do que cortar se atrasar

```
NUNCA CORTAR (aprovação em risco):
  ❌ Sensor SPI bare-metal (é o que substitui interface de comunicação)
  ❌ ZBus (requisito explícito — ao menos um canal publicando)
  ❌ Logging (requisito explícito — já está implementado)

MANTER SE POSSÍVEL:
  ⚡ FSM com 3 estados (pode simplificar para ARMED/DISARMED se necessário)
  ⚡ Shell (pelo menos safeguard status + safeguard threshold)
  ⚡ Settings (threshold persistido — o caso mais simples)
  ⚡ Botão (pode virar trigger direto sem timer se long press atrasar)

SIMPLIFICAR SE ATRASAR:
  ⚠️  Testes: 2 casos passando > 5 casos quebrados
  ⚠️  LOG_WRN de alerta precoce: opcional se o LOG_ERR de alarme já funcionar
  ⚠️  event_count no shell status: pode ser removido sem impacto no requisito
  ⚠️  safeguard arm/disarm no shell: botão físico já cobre o requisito
```

---

### Checklist de "definition of done"

Use antes da entrega final:

- [ ] `west build -b nucleo_f401re safeguard-node --build-dir safeguard-node/build -p always` compila sem warnings
- [ ] `west flash` → boot sem erros, "SafeGuard Node iniciado" aparece no log
- [ ] Botão longo → LED acende (ARMADO); botão longo → LED apaga (DESARMADO)
- [ ] Bater na mesa com sistema armado → LED pisca, LOG_ERR no console
- [ ] `safeguard status` responde com estado e threshold
- [ ] `safeguard threshold 150` + reboot → threshold ainda é 150
- [ ] `west twister -T adxl345_spi/tests -p qemu_cortex_m3` → pelo menos 3 testes PASSED
- [ ] Commits semânticos em branches por feature no repositório Git
- [ ] README com instruções de build, wiring, shell e testes

---

### Comandos de referência rápida

```bash
# Ambiente (rodar sempre que abrir terminal novo)
source /zephyr/.venv/bin/activate
source /zephyr/zephyr/zephyr-env.sh
cd /zephyr

# Build — hardware real
west build -b nucleo_f401re safeguard-node --build-dir safeguard-node/build
west build -b nucleo_f401re safeguard-node --build-dir safeguard-node/build -p always

# Build — emulação (macOS, sem hardware)
west build -b mps2/an385 safeguard-node --build-dir safeguard-node/build
west build --build-dir safeguard-node/build -t run   # QEMU: sair com Ctrl+A → X

# Flash
west flash

# Monitor serial
screen /dev/tty.usbmodem* 115200
# sair do screen: Ctrl+A, depois K, depois Y

# Testes
west twister -T safeguard-node/tests -p qemu_cortex_m3 --inline-logs

# Ver Kconfig atual
west build -t menuconfig
```
