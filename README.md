# SafeGuard Node

Nó embarcado de monitoramento de segurança física para gabinetes críticos, implementado com **Zephyr RTOS** e driver SPI bare-metal para o acelerômetro ADXL345.

Quando armado, o dispositivo detecta qualquer impacto ou movimento acima do threshold configurado e dispara um alarme visual. Controle via botão físico e shell UART; configuração persistida em flash NVS.

**Plataforma:** ST Nucleo-F401RE (STM32F401, Cortex-M4)  
**RTOS:** Zephyr 4.3.99  
**Sensor:** ADXL345 via SPI1 — driver bare-metal (sem `CONFIG_ADXL345`)

---

## Estados operacionais

| Estado | LED | Condição |
|--------|-----|----------|
| **DESARMADO** | Apagado | Sistema ativo, alarme inativo |
| **ARMADO** | Aceso fixo | Baseline capturado, detectando movimento |
| **ALARME** | Pisca 5 Hz | Delta de aceleração acima do threshold detectado |

Transições são disparadas por **pressão longa no botão** (> 1,5 s) ou pelos comandos `safeguard arm` / `safeguard disarm` no shell.

---

## Hardware

### Componentes

- ST Nucleo-F401RE (LED `LD2` e botão `USER` onboard)
- Módulo ADXL345

### Fiação — SPI1 nos headers Arduino R3

| Sinal | Header | Pino MCU |
|-------|--------|----------|
| SCK   | D13    | PA5      |
| MISO  | D12    | PA6      |
| MOSI  | D11    | PA7      |
| CS    | D10    | PB6      |
| VCC   | 3.3V   | —        |
| GND   | GND    | —        |

---

## Arquitetura de software

### Threads e canais ZBus

```
sensor_thread ──► accel_data ──► proc_thread ──► system_state ──► led_thread
                                     ▲                                
                               arm_event ◄── button_isr               
                               arm_event ◄── shell_thread             
```

| Canal ZBus | Produtor | Consumidor | Conteúdo |
|------------|----------|------------|----------|
| `accel_data` | `sensor_thread` | `proc_thread` | `int16_t x, y, z` brutos |
| `system_state` | `proc_thread` | `led_thread` | `enum sg_state` + `event_count` |
| `arm_event` | `button_isr`, shell | `proc_thread` | `enum sg_cmd` (ARM / DISARM) |

### Detecção de impacto

Ao armar, `proc_thread` captura um baseline XYZ. A cada leitura:

```
delta² = (x - bx)² + (y - by)² + (z - bz)²
```

Se `delta² > threshold²` → transição para ALARME. Aritmética inteira pura, sem ponto flutuante.

### Configuração (Kconfig)

| Opção | Padrão | Descrição |
|-------|--------|-----------|
| `CONFIG_SG_SAMPLE_INTERVAL_MS` | 50 | Período de leitura do sensor (ms) |
| `CONFIG_SG_BUTTON_LONG_PRESS_MS` | 1500 | Duração mínima para arme/desarme (ms) |
| `CONFIG_SG_THRESHOLD_DEFAULT_LSB` | 80 | Threshold padrão de fábrica (~0,3 g) |
| `CONFIG_SG_WARN_THRESHOLD_PERCENT` | 80 | % do threshold que emite `LOG_WRN` |

---

## Build e flash

### Pré-requisitos

```sh
# Workspace Zephyr em /zephyr
source .venv/bin/activate
source zephyr/zephyr-env.sh
```

### Compilar para a placa

```sh
# A partir da raiz do workspace
west build -b nucleo_f401re safeguard-node --build-dir safeguard-node/build

# Pristine (obrigatório após mudar .conf, .overlay ou board)
west build -b nucleo_f401re safeguard-node --build-dir safeguard-node/build -p always
```

### Emulação sem hardware (macOS/Linux)

`native_sim` só funciona no Linux. No macOS, use `mps2/an385` (Cortex-M3 via QEMU):

```sh
# Build para emulação
west build -b mps2/an385 safeguard-node --build-dir safeguard-node/build

# Rodar no QEMU (sair: Ctrl+A → X)
west build --build-dir safeguard-node/build -t run
```

> O QEMU emula o tempo mais rápido que o relógio de parede — os timestamps no log estão corretos, mas o tempo real decorrido será menor.

### Gravar na placa

```sh
west flash   # via ST-LINK
```

### Console serial

```sh
screen /dev/tty.usbmodem* 115200
# Sair: Ctrl+A → K → Y
```

---

## Uso

### Shell UART

Conecte ao console serial e use o prefixo `safeguard`:

```
uart:~$ safeguard status
Estado: ARMADO | Threshold: 80 LSB | Eventos: 2

uart:~$ safeguard arm
uart:~$ safeguard disarm

uart:~$ safeguard threshold 120
Threshold atualizado: 120 LSB (salvo em NVS)

uart:~$ safeguard reset
Configuração restaurada para padrão de fábrica
```

### Botão físico (USER button — PC13)

| Ação | Efeito |
|------|--------|
| Pressão longa > 1,5 s em DESARMADO | → ARMADO (baseline capturado) |
| Pressão longa > 1,5 s em ARMADO | → DESARMADO |
| Pressão longa > 1,5 s em ALARME | → DESARMADO (reconhece o evento) |

### Persistência

O threshold configurado via shell é salvo automaticamente em NVS (`sg/threshold`) e restaurado no próximo boot. Para limpar, use `safeguard reset`.

---

## Testes

A suite ZTEST cobre as funções críticas como código puro, sem dependência de hardware — roda em QEMU.

```sh
west twister -T safeguard-node/tests -p qemu_cortex_m3 --inline-logs
```

### Casos de teste

| Caso | Tipo | O que valida |
|------|------|--------------|
| `test_no_impact_below_threshold` | Unitário | `sg_detect_impact()` retorna false quando delta < threshold |
| `test_impact_above_threshold` | Unitário | `sg_detect_impact()` retorna true quando delta > threshold |
| `test_fsm_arm_transition` | Unitário | DISARMED + CMD_ARM → ARMED |
| `test_fsm_alarm_on_impact` | Unitário | ARMED + CMD_IMPACT → ALARM |
| `test_settings_round_trip` | Integração | threshold salvo == threshold lido após `settings_load()` |

---

## Estrutura do repositório

```
safeguard-node/
├── src/
│   ├── main.c              # boot: settings_load, criação de threads
│   ├── sensor.c / .h       # sg_sensor_thread: lê ADXL345, publica accel_data
│   ├── proc.c / .h         # sg_proc_thread: FSM + sg_detect_impact()
│   ├── led.c / .h          # sg_led_thread: padrão de pisca por estado
│   ├── button.c / .h       # ISR + long press → publica arm_event
│   ├── shell_cmd.c         # comandos safeguard *
│   ├── settings_nvs.c      # sg_settings_load / sg_settings_save
│   └── zbus_channels.c     # ZBUS_CHAN_DEFINE dos 3 canais
├── tests/
│   ├── src/
│   │   ├── test_detect.c   # unitários: detect + FSM
│   │   └── test_settings.c # integração: NVS round-trip
│   ├── CMakeLists.txt
│   └── prj.conf
├── boards/
│   └── nucleo_f401re.overlay  # adxl345@0 em &spi1
├── docs/
├── Kconfig.safeguard       # opções CONFIG_SG_*
├── CMakeLists.txt
├── prj.conf
└── README.md
```

---

## Requisitos da disciplina

| Requisito | Como é atendido |
|-----------|-----------------|
| UART de debug | UART2 via ST-LINK — log de estados e shell na mesma porta |
| LED | LD2 onboard — 3 padrões mapeados para os estados da FSM |
| Botão de função | USER button onboard — long press para arme/desarme |
| Sensor SPI sem driver pronto | ADXL345 via `spi_transceive_dt()`, `CONFIG_ADXL345` desabilitado |
| C Code Guidelines Zephyr | Prefixo `sg_`, `static` em funções internas, enums nomeados, sem magic numbers |
| ZTEST + Twister | 5 casos de teste, roda em `qemu_cortex_m3` |
| Logging | `LOG_INF` em transições, `LOG_WRN` em alerta precoce, `LOG_ERR` em alarme |
| Shell | 5 subcomandos `safeguard *` registrados com `SHELL_CMD_REGISTER` |
| Settings | Threshold persistido em NVS, restaurado no boot |
| ZBus | 3 canais desacoplando sensor, FSM, LED e botão |
