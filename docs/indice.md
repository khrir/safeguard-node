# Documentação — SafeGuard Node

---

## Referência rápida

| Documento | Para quem | O que responde |
|-----------|-----------|----------------|
| [Domínio](dominio.md) | Desenvolvedor | O que o sistema faz, as regras e as entidades |
| [Diagramas](diagramas.md) | Desenvolvedor | Como as partes interagem em cada fluxo |
| [ADR-001](adr/ADR-001-zephyr-rtos.md) | Desenvolvedor | Por que Zephyr RTOS |
| [ADR-002](adr/ADR-002-spi-bare-metal.md) | Desenvolvedor | Por que SPI bare-metal (sem CONFIG_ADXL345) |
| [ADR-003](adr/ADR-003-zbus-pubsub.md) | Desenvolvedor | Por que ZBus e não message queue |
| [ADR-004](adr/ADR-004-aritmetica-inteira.md) | Desenvolvedor | Por que aritmética inteira na detecção |
| [ADR-005](adr/ADR-005-nvs-settings.md) | Desenvolvedor | Por que Settings NVS para persistência |
| [ADR-006](adr/ADR-006-emulacao-mps2-an385.md) | Desenvolvedor | Por que mps2/an385 no macOS |
| [Glossário](glossario.md) | Qualquer pessoa | O que cada termo técnico significa |
| [Manual de execução](manual_execucao.md) | Operador / desenvolvedor | Como buildar, rodar, usar o shell e testar |
| [Roteiro de apresentação](roteiro_proposta_e_implementacao.md) | Apresentador | Script e guia de implementação |

---

## Decisões arquiteturais (ADRs)

Os ADRs registram **por que** cada decisão foi tomada, não apenas **o quê**. Leia-os quando:
- Precisar justificar uma escolha para o professor ou colega.
- Avaliar se uma decisão ainda faz sentido após mudança de requisito.
- Entender o que acontece se uma peça for substituída.

---

## Artefatos de apresentação

| Arquivo | Tipo |
|---------|------|
| [safeguard_node_proposta.pptx](safeguard_node_proposta.pptx) | Slides da defesa de proposta (19/06/2026) |
| [gen_slides.js](gen_slides.js) | Script que gerou os slides (pptxgenjs) |
| [roteiro_proposta_e_implementacao.md](roteiro_proposta_e_implementacao.md) | Script de fala + plano de implementação dia a dia |
