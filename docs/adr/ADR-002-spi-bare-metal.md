# ADR-002 — Driver SPI bare-metal (CONFIG_ADXL345 desabilitado)

**Status:** Aceito  
**Data:** 2026-06-18

---

## Contexto

O Zephyr possui um driver de sensor completo para o ADXL345 (`CONFIG_ADXL345=y`, subsistema `zephyr/drivers/sensor/adxl345`). Esse driver expõe uma API de alto nível (`sensor_sample_fetch`, `sensor_channel_get`) que abstrai completamente o protocolo SPI.

O projeto tem dois objetivos: (1) requisito explícito da disciplina de construir uma interface de comunicação "na mão"; (2) tornar o tráfego SPI visível e analisável com osciloscópio ou analisador lógico.

## Decisão

Manter `CONFIG_ADXL345` **desabilitado**. Implementar as transações SPI diretamente via `spi_transceive_dt()` e `spi_write_dt()`, montando os bytes do protocolo ADXL345 manualmente no código.

## Detalhes de implementação

- O nó `adxl345@0` no overlay usa `compatible = "adi,adxl345"` apenas para satisfazer o binding do Zephyr — nenhum driver se acopla a ele.
- `SPI_DT_SPEC_GET` resolve a configuração de hardware em tempo de compilação a partir do devicetree.
- Protocolo implementado: modo 3 (CPOL=1, CPHA=1), MSB first, ≤5 MHz. Byte de endereço: bit7=R/W, bit6=MB (multi-byte), bits5-0=registrador.
- O locking do barramento é gerenciado pelo driver SPI do Zephyr internamente — não é necessário mutex adicional no código da aplicação.

## Consequências

**Positivas:**
- Cada byte trafegado no barramento é rastreável — o tráfego é idêntico ao que um engenheiro veria no osciloscópio.
- Demonstra domínio do protocolo SPI no nível elétrico.
- Atende ao requisito de "interface de comunicação implementada na mão".
- Código mais simples de entender para fins didáticos — não há indireção de callbacks do subsistema de sensores.

**Negativas:**
- Não reutiliza os testes e a manutenção do driver in-tree. Bugs no protocolo são responsabilidade do projeto.
- Se o ADXL345 for substituído por outro sensor, o driver precisa ser reescrito do zero.
- `rx[0]` é sempre descartado (contém o clock-in do byte de endereço) — detalhe não óbvio para leitores futuros.

## Alternativas consideradas

| Alternativa | Motivo de rejeição |
|-------------|-------------------|
| `CONFIG_ADXL345=y` com API de sensor | Abstrai o protocolo — impossível ver os bytes no osciloscópio; não atende o requisito da disciplina |
| I2C em vez de SPI | ADXL345 suporta I2C, mas o foco didático é SPI; fiação Arduino R3 já está definida para SPI1 |
