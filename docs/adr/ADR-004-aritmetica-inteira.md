# ADR-004 — Aritmética inteira pura na detecção de impacto

**Status:** Aceito  
**Data:** 2026-06-18

---

## Contexto

O cálculo central de detecção compara a distância euclidiana entre a leitura atual e o baseline:

```
distância = √((x-bx)² + (y-by)² + (z-bz)²)
```

Essa operação pode ser implementada com ponto flutuante (mais legível) ou com aritmética inteira (mais eficiente e previsível).

O STM32F401 tem unidade de ponto flutuante em hardware (FPU Cortex-M4), portanto a operação float não seria proibitivamente cara.

## Decisão

Evitar a raiz quadrada comparando `delta²` com `threshold²` diretamente, usando **aritmética inteira de 32 bits**:

```c
int32_t dx = cur->x - base->x;
int32_t dy = cur->y - base->y;
int32_t dz = cur->z - base->z;
uint32_t delta_sq = (uint32_t)(dx*dx + dy*dy + dz*dz);
return delta_sq > (uint32_t)(threshold_lsb * threshold_lsb);
```

## Análise de overflow

- Valores XYZ em full-resolution ±16g: intervalo ±4096 LSB.
- `dx` máximo: 8192 (diferença entre extremos).
- `dx²` máximo: 67.108.864 (< 2³²/4 = 1.073.741.824).
- `dx² + dy² + dz²` máximo: ~201.326.592 — cabe em `uint32_t` com margem.
- `threshold` máximo configurável: 1000 LSB → `threshold²` = 1.000.000 — cabe em `uint32_t`.

Não há risco de overflow dentro do intervalo de entrada válido.

## Consequências

**Positivas:**
- Elimina a operação `sqrtf()` — reduz ciclos de CPU na `proc_thread`.
- Sem dependência de estado de FPU — contexto de thread mais simples (Zephyr salva/restaura FPU, mas evitar o uso reduz essa carga quando há muitas threads).
- Resultado determinístico — sem variação de arredondamento de ponto flutuante entre compilações ou plataformas.
- Facilita o port para microcontroladores sem FPU (ex: Cortex-M0).

**Negativas:**
- O threshold configurado pelo usuário (em LSB) é comparado como `threshold²` internamente — a escala não é linear. Um usuário que dobra o threshold não está dobrando a sensibilidade em termos de distância, mas sim ampliando a área de detecção por um fator de 4. Isso deve ser documentado no shell e no glossário.
- Menos legível que a versão com `sqrtf()` para leitores sem familiaridade com a otimização.

## Alternativas consideradas

| Alternativa | Motivo de rejeição |
|-------------|-------------------|
| `sqrtf(dx*dx + dy*dy + dz*dz)` com float | Correto, mas introduz FPU e não há ganho funcional no contexto |
| Comparação eixo a eixo (`|dx| > thr && |dy| > thr`) | Falsos negativos em impactos diagonais — geometricamente incorreto |
| CMSIS DSP `arm_sqrt_q31` | Dependência adicional sem benefício real para este caso |
