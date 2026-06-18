# Diagramas de Interação — SafeGuard Node

Diagramas em sintaxe Mermaid (renderizados no GitHub e em editores compatíveis).

---

## 1. Boot e inicialização

Sequência executada uma vez na inicialização do firmware.

```mermaid
sequenceDiagram
    participant K  as Zephyr Kernel
    participant M  as main()
    participant NVS as Settings NVS
    participant ZB as ZBus
    participant ST as sensor_thread
    participant PT as proc_thread
    participant LT as led_thread
    participant BT as button_isr

    K->>M: chama main()
    M->>NVS: settings_subsys_init()
    M->>NVS: settings_load()
    NVS-->>M: threshold_lsb (ou padrão)
    M->>ZB: inicializa canais (accel_data, system_state, arm_event)
    M->>ST: k_thread_create(sensor_thread)
    M->>PT: k_thread_create(proc_thread)
    M->>LT: k_thread_create(led_thread)
    M->>BT: gpio_pin_interrupt_configure()
    M->>M: LOG_INF("SafeGuard Node iniciado")
    Note over K: scheduler assume controle
```

---

## 2. Arme via botão físico (long press)

```mermaid
sequenceDiagram
    participant U  as Usuário
    participant HW as Botão (PC13)
    participant IS as button_isr
    participant PT as proc_thread
    participant ZB as ZBus (arm_event)
    participant LT as led_thread
    participant ZS as ZBus (system_state)

    U->>HW: pressiona botão
    HW->>IS: GPIO_INT_EDGE_FALLING
    IS->>IS: t_press = k_uptime_get()
    Note over IS: ISR retorna imediatamente

    U->>HW: solta botão (após ≥1500 ms)
    HW->>IS: GPIO_INT_EDGE_RISING
    IS->>IS: delta = k_uptime_get() - t_press
    alt delta >= 1500 ms
        IS->>ZB: zbus_chan_pub(arm_event, CMD_ARM)
    else delta < 1500 ms
        IS->>IS: ignora (pressão curta)
    end

    ZB->>PT: notifica (subscriber)
    PT->>PT: captura baseline XYZ
    PT->>PT: state = ARMED
    PT->>ZS: zbus_chan_pub(system_state, ARMED)
    ZS->>LT: notifica
    LT->>LT: acende LED fixo
```

---

## 3. Detecção de impacto

```mermaid
sequenceDiagram
    participant HW as ADXL345 (SPI)
    participant ST as sensor_thread
    participant ZA as ZBus (accel_data)
    participant PT as proc_thread
    participant ZS as ZBus (system_state)
    participant LT as led_thread

    loop a cada 50 ms
        ST->>HW: spi_transceive_dt() — lê 6 bytes XYZ
        HW-->>ST: rx[1..6] = X0,X1,Y0,Y1,Z0,Z1
        ST->>ZA: zbus_chan_pub(accel_data, {x,y,z})
        ZA->>PT: notifica

        PT->>PT: delta² = (x-bx)²+(y-by)²+(z-bz)²
        alt state == ARMED
            alt delta² > threshold²
                PT->>PT: state = ALARM, event_count++
                PT->>PT: LOG_ERR("violação detectada")
                PT->>ZS: pub(system_state, ALARM)
                ZS->>LT: notifica
                LT->>LT: pisca LED a 5 Hz
            else delta² > warn²
                PT->>PT: LOG_WRN("delta próximo do threshold")
            end
        end
    end
```

---

## 4. Comando shell — alteração de threshold

```mermaid
sequenceDiagram
    participant Op as Operador (UART)
    participant SH as shell_thread
    participant PT as proc_thread
    participant NVS as Settings NVS

    Op->>SH: safeguard threshold 150
    SH->>SH: valida: 10 ≤ 150 ≤ 1000 ✓
    SH->>PT: atualiza threshold_lsb = 150
    SH->>NVS: settings_save_one("sg/threshold", 150)
    NVS-->>SH: ok
    SH->>Op: "Threshold atualizado: 150 LSB (salvo em NVS)"

    Note over Op,NVS: após reboot
    Note over NVS: settings_load() restaura 150
```

---

## 5. Reconhecimento de alarme via shell

```mermaid
sequenceDiagram
    participant Op as Operador (UART)
    participant SH as shell_thread
    participant ZB as ZBus (arm_event)
    participant PT as proc_thread
    participant ZS as ZBus (system_state)
    participant LT as led_thread

    Note over LT: LED piscando (ALARM)
    Op->>SH: safeguard disarm
    SH->>ZB: pub(arm_event, CMD_DISARM, SOURCE_SHELL)
    ZB->>PT: notifica
    PT->>PT: state = DISARMED
    PT->>ZS: pub(system_state, DISARMED)
    ZS->>LT: notifica
    LT->>LT: apaga LED
    SH->>Op: LOG_INF("sistema desarmado")
```

---

## 6. Fluxo completo de estados

```mermaid
stateDiagram-v2
    [*] --> DISARMED : boot

    DISARMED --> ARMED : CMD_ARM\n(botão long press ou shell)
    ARMED --> DISARMED : CMD_DISARM
    ARMED --> ALARM : delta² > threshold²
    ALARM --> DISARMED : CMD_DISARM\n(reconhece o evento)

    state DISARMED {
        direction LR
        [*] --> idle : LED apagado
    }

    state ARMED {
        direction LR
        [*] --> monitorando : LED fixo\nbaseline capturado
    }

    state ALARM {
        direction LR
        [*] --> alertando : LED 5 Hz\nLOG_ERR emitido
    }
```

---

## 7. Arquitetura de threads e canais (visão estática)

```
┌─────────────────┐    accel_data     ┌─────────────────┐    system_state   ┌─────────────────┐
│  sensor_thread  │ ─────────────────►│  proc_thread    │ ─────────────────►│   led_thread    │
│                 │   {x, y, z}       │                 │   {state, count}  │                 │
│  spi_transceive │                   │  FSM + detect   │                   │  LED pattern    │
│  50 ms period   │                   │  baseline       │                   │  by state       │
└─────────────────┘                   └────────┬────────┘                   └─────────────────┘
                                               │ arm_event
                                               │ {CMD_ARM / CMD_DISARM}
                                    ┌──────────┴──────────┐
                              ┌─────┴─────┐         ┌─────┴──────┐
                              │button_isr │         │shell_thread│
                              │ long press│         │ safeguard *│
                              └───────────┘         └────────────┘
```
