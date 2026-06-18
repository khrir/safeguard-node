# ADR-003 — ZBus como barramento de eventos entre threads

**Status:** Aceito  
**Data:** 2026-06-18

---

## Contexto

O sistema tem quatro agentes concorrentes que precisam trocar informação: `sensor_thread`, `proc_thread`, `led_thread` e `button_isr/shell`. As opções de comunicação entre threads no Zephyr incluem: message queues (`k_msgq`), semáforos, mailboxes, pipes e o ZBus.

O requisito da disciplina menciona explicitamente o uso de ZBus.

## Decisão

Usar o subsistema **ZBus** com três canais tipados:

| Canal | Tipo | Produtor | Consumidor |
|-------|------|----------|------------|
| `accel_data` | `struct sg_accel {int16_t x,y,z}` | `sensor_thread` | `proc_thread` |
| `system_state` | `struct sg_state_msg {enum sg_state, uint32_t event_count, uint16_t threshold_lsb}` | `proc_thread` | `led_thread` |
| `arm_event` | `struct sg_arm_event {enum sg_cmd, enum sg_source}` | `button_isr`, `shell_thread` | `proc_thread` |

## Consequências

**Positivas:**
- **Desacoplamento**: `sensor_thread` não sabe que `proc_thread` existe — apenas publica. Adicionar um subscriber (ex: buzzer, BLE) exige zero mudança no produtor.
- **Tipagem em tempo de compilação**: o canal carrega um tipo específico — impossível publicar `arm_event` em `accel_data` por engano.
- **Testabilidade**: cada thread pode ser testada de forma isolada publicando mensagens sintéticas no canal.
- **Sem polling**: `proc_thread` bloqueia esperando notificação do ZBus — não consome CPU enquanto não há dado novo.

**Negativas:**
- ZBus **não** garante isolamento de falha. Se `sensor_thread` travar, `proc_thread` para de receber dados mas não é notificada. Detecção de falha exigiria watchdog adicional (fora do escopo).
- Overhead de memória ligeiramente maior que `k_msgq` para canais com único subscriber.
- A semântica de "último valor" do ZBus (SPSC por padrão) precisa ser verificada para o canal `arm_event` — dois comandos rápidos consecutivos podem colidir.

## Alternativas consideradas

| Alternativa | Motivo de rejeição |
|-------------|-------------------|
| `k_msgq` (message queue) | Sem tipo estático garantido; não é pub-sub — acoplamento 1:1 produtor/consumidor |
| Variáveis globais com mutex | Solução correta mas muito mais verbosa; sem infraestrutura de notificação |
| Chamada direta de função entre threads | Cria dependência de compilação e pode causar deadlock se chamada de contextos diferentes |
| Pipes Zephyr | Para dados binários contínuos, não para eventos tipados |
