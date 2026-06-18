# ADR-001 — Uso do Zephyr RTOS como plataforma

**Status:** Aceito  
**Data:** 2026-06-18

---

## Contexto

O projeto exige múltiplas tarefas concorrentes com prazos distintos: leitura de sensor a 50 ms, resposta a interrupção de botão, controle de LED com temporização precisa (5 Hz), shell interativo e persistência em NVS. Uma implementação bare-metal sem RTOS exigiria um superloop com máquina de estados manual, gerenciamento de tempo via SysTick e código altamente acoplado.

O requisito da disciplina menciona explicitamente o uso do Zephyr e de subsistemas como ZBus, Settings e ZTEST.

## Decisão

Usar o **Zephyr RTOS 4.3.99** como sistema operacional do firmware.

## Consequências

**Positivas:**
- Scheduler preemptivo garante que a leitura do sensor (50 ms) não é bloqueada por operações de shell ou NVS.
- ZBus, Settings, Shell e ZTEST são subsistemas prontos — evita implementar infraestrutura de comunicação e teste do zero.
- `spi_transceive_dt()` encapsula locking de barramento — múltiplas threads não corrompem o SPI.
- Devicetree + Kconfig garantem que a configuração de hardware é verificada em tempo de compilação, não em tempo de execução.

**Negativas:**
- Overhead de memória: o kernel Zephyr mínimo ocupa ~8 KB de RAM e ~22 KB de flash (valores medidos no build `mps2/an385`).
- Curva de aprendizagem para Kconfig, devicetree e ZBus.
- Builds são mais lentos que um projeto bare-metal equivalente.

## Alternativas consideradas

| Alternativa | Motivo de rejeição |
|-------------|-------------------|
| FreeRTOS | Não tem ZBus, Settings ou ZTEST; exigiria mais infraestrutura manual |
| Bare-metal (superloop) | Inviável para os requisitos de concorrência e persistência |
| Arduino framework | Não tem threading real nem suporte adequado ao STM32F401 |
