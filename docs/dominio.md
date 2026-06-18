# Camada de Domínio — SafeGuard Node

Este documento descreve o modelo de domínio do sistema: as entidades que existem, as regras que governam seu comportamento e a linguagem que dá nome às coisas.

---

## Linguagem Ubíqua

Termos usados de forma consistente em código, comentários, logs e documentação.

| Termo | Significado no domínio |
|-------|------------------------|
| **Nó** | O dispositivo embarcado completo (hardware + firmware) |
| **Armar** | Ativar a vigilância: capturar baseline e começar a detectar |
| **Desarmar** | Desativar a vigilância sem registrar evento |
| **Baseline** | Leitura XYZ capturada no momento do arme; serve de referência |
| **Delta** | Diferença vetorial entre a leitura atual e o baseline |
| **Threshold** | Limiar de delta acima do qual o sistema considera um impacto |
| **Impacto** | Evento de movimentação acima do threshold detectado em modo armado |
| **Alarme** | Estado ativo após detecção de impacto; persistido até desarme explícito |
| **Evento** | Instância de impacto registrada; incrementa o contador de eventos |
| **Long press** | Pressão do botão físico por mais de 1,5 s; única forma de armar/desarmar via hardware |
| **Persistência** | Gravação do threshold em flash NVS para sobreviver a reinicializações |
| **Leitura bruta** | Valor XYZ em LSB diretamente do sensor, sem conversão para unidades físicas |

---

## Entidades do Domínio

### `AccelReading`
Leitura bruta do acelerômetro. Produzida pelo sensor a cada ciclo de amostragem.

```
AccelReading {
  x: int16   — eixo X em LSB (little-endian, do registrador DATAX0)
  y: int16   — eixo Y em LSB
  z: int16   — eixo Z em LSB
}
```

Unidade implícita: **3,9 mg/LSB** em modo full-resolution ±16g (DATA_FORMAT = 0x0B).

---

### `Baseline`
Snapshot de `AccelReading` capturado no instante em que o sistema transita para ARMED. Representa o estado estático esperado do objeto monitorado.

```
Baseline {
  reading: AccelReading   — leitura no instante do arme
  captured_at: uptime_ms  — timestamp Zephyr para fins de log
}
```

Regra: **o baseline é descartado ao desarmar**. Um novo arme sempre gera um novo baseline.

---

### `SystemState`
Estado operacional corrente do sistema. Publicado no canal ZBus `system_state` sempre que houver transição.

```
SystemState {
  state: enum { DISARMED, ARMED, ALARM }
  event_count: uint32   — total de impactos desde o último arme
  threshold_lsb: uint16 — threshold vigente no momento da publicação
}
```

---

### `ArmEvent`
Comando de controle emitido pelo botão físico ou pelo shell. Lido pela `proc_thread`.

```
ArmEvent {
  cmd: enum { CMD_ARM, CMD_DISARM }
  source: enum { SOURCE_BUTTON, SOURCE_SHELL }
}
```

---

### `Threshold`
Limiar de sensibilidade configurado pelo operador. Persiste entre reinicializações.

```
Threshold {
  value_lsb: uint16        — valor em LSB (padrão de fábrica: 80)
  warn_percent: uint8      — % do threshold que emite LOG_WRN (padrão: 80)
  nvs_key: "sg/threshold"  — chave de persistência em NVS
}
```

Intervalo válido: **10–1000 LSB**. Valores fora do intervalo são rejeitados pelo shell.

---

## Regras de Domínio

### R1 — Cálculo de delta
```
delta² = (x - bx)² + (y - by)² + (z - bz)²
```
Compara-se `delta²` com `threshold²` para evitar a operação de raiz quadrada. Aritmética 32-bit inteira pura; sem ponto flutuante.

### R2 — Condição de alarme
```
se delta² > threshold²  →  transitar para ALARM, incrementar event_count
```

### R3 — Alerta precoce
```
se delta² > (threshold × warn_percent / 100)²  →  emitir LOG_WRN
```
Emitido antes do alarme para dar visibilidade de vibração próxima do limiar.

### R4 — Transições de estado válidas

```
DISARMED ──[CMD_ARM]──────► ARMED
ARMED    ──[CMD_DISARM]───► DISARMED
ARMED    ──[impacto]──────► ALARM
ALARM    ──[CMD_DISARM]───► DISARMED   (reconhece o evento)
```

Transições inválidas (ex: CMD_ARM em estado ARMED) são ignoradas silenciosamente.

### R5 — Long press do botão
A ISR registra apenas o timestamp de borda de descida. Uma thread separada compara com o timestamp de borda de subida. Se `(t_release - t_press) >= 1500 ms` → emite `ArmEvent`. A ISR nunca bloqueia.

### R6 — Persistência do threshold
O threshold é salvo em NVS imediatamente após cada alteração via shell. No boot, `settings_load()` restaura o último valor salvo. Se não houver valor salvo, usa o padrão de fábrica definido em Kconfig (`CONFIG_SG_THRESHOLD_DEFAULT_LSB`).

---

## Invariantes

- O sistema **nunca** transita de DISARMED direto para ALARM.
- O `event_count` **nunca** decrementa; é zerado apenas por `safeguard reset`.
- O baseline **nunca** é atualizado enquanto o sistema está em ARMED (re-arme exige DISARM primeiro).
- O threshold persiste entre reinicializações; nunca volta ao padrão sem comando explícito.
