# ADR-006 — mps2/an385 como plataforma de emulação (macOS)

**Status:** Aceito  
**Data:** 2026-06-18

---

## Contexto

O hardware alvo (Nucleo-F401RE) não tem suporte QEMU no Zephyr — é uma placa real que requer ST-LINK para programação. Durante o desenvolvimento sem o hardware disponível, é necessária uma plataforma de emulação para validar o ciclo de build, o shell, o ZBus e os testes.

O Zephyr oferece duas opções principais de emulação:
- `native_sim`: compila o firmware como processo Linux nativo — sem QEMU, mais rápido.
- `mps2/an385`: Cortex-M3 emulado via QEMU — mais próximo de um MCU real.

## Decisão

Usar **`mps2/an385`** (ARM MPS2 AN385, Cortex-M3) como plataforma de emulação.

`native_sim` foi descartado porque **só funciona em Linux**. O ambiente de desenvolvimento é macOS (Darwin 24.6.0), e `native_sim` falha na configuração CMake com o erro:

```
CMake Error: The POSIX architecture only works on Linux.
```

## Consequências

**Positivas:**
- Roda em macOS via `qemu-system-arm` (disponível via Homebrew).
- Console serial funciona no mesmo terminal — sem configuração adicional.
- O Zephyr kernel, ZBus, shell e Settings funcionam identicamente ao alvo real.
- Suficiente para validar toda a lógica de software exceto a leitura SPI real.

**Negativas:**
- `mps2/an385` é Cortex-M3, não Cortex-M4 — a instrução `sqrtf()` via FPU não está disponível na emulação (irrelevante, pois o projeto usa aritmética inteira — ADR-004).
- SPI não está conectado a nada — `spi_transceive_dt()` falha ou retorna erro silencioso. A leitura do ADXL345 precisa de stub para emulação.
- O QEMU emula o tempo mais rápido que o relógio de parede — timestamps no log estão corretos, mas o tempo decorrido real é menor.
- `west build -t run` mantém o processo QEMU vivo indefinidamente. Para sair: `Ctrl+A → X`.

## Impacto na estrutura do código

A `sensor_thread` precisa de uma implementação de stub para emulação:

```c
#ifdef CONFIG_BOARD_MPS2_AN385
/* stub: retorna valores fixos para desenvolvimento sem hardware */
static int sg_sensor_read(struct sg_accel *out) {
    out->x = 0; out->y = 0; out->z = 256; /* ≈1g no eixo Z */
    return 0;
}
#else
/* produção: lê ADXL345 via SPI */
static int sg_sensor_read(struct sg_accel *out) {
    return adxl345_read_xyz(&out->x, &out->y, &out->z);
}
#endif
```

## Alternativas consideradas

| Alternativa | Motivo de rejeição |
|-------------|-------------------|
| `native_sim` | Não funciona no macOS — POSIX arch requer Linux |
| `qemu_cortex_m3` (alias antigo) | Renomeado para `mps2/an385` no Zephyr 4.x — o alias não existe mais |
| VM Linux + `native_sim` | Adiciona complexidade de setup; `mps2/an385` é mais simples |
| Sem emulação (só hardware real) | Bloqueia desenvolvimento enquanto o hardware não está disponível |
