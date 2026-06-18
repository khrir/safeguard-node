# ADR-005 — Persistência de configuração via Settings NVS

**Status:** Aceito  
**Data:** 2026-06-18

---

## Contexto

O threshold configurado pelo usuário via shell deve sobreviver a reinicializações. Sem persistência, o operador precisaria reconfigurar o dispositivo após cada reboot.

O Zephyr oferece o subsistema `Settings` como camada de abstração sobre diferentes backends de armazenamento. O backend mais simples e adequado para microcontroladores sem sistema de arquivos é o NVS (Non-Volatile Storage), que opera diretamente na flash interna com um esquema de chave-valor tolerante a falhas de energia.

## Decisão

Usar `CONFIG_SETTINGS=y` com `CONFIG_SETTINGS_NVS=y` como backend.

Chave de armazenamento: `"sg/threshold"` (namespace `sg`, chave `threshold`).

Fluxo:
1. No boot: `settings_subsys_init()` + `settings_load()` — restaura o threshold salvo ou usa o padrão do Kconfig.
2. Ao alterar via shell: `settings_save_one("sg/threshold", &val, sizeof(val))` — gravação imediata, sem buffer.
3. `safeguard reset`: apaga a chave NVS e restaura `CONFIG_SG_THRESHOLD_DEFAULT_LSB`.

## Consequências

**Positivas:**
- API de alto nível — o código não gerencia endereços de flash manualmente.
- NVS é tolerante a falhas de energia: usa um esquema de write-once com garbage collection que garante que uma reinicialização durante a escrita não corrompe o valor anterior.
- Fácil de estender: adicionar novos parâmetros persistidos exige apenas uma nova chave (ex: `"sg/event_count"`).

**Negativas:**
- Requer `CONFIG_FLASH=y`, `CONFIG_FLASH_MAP=y` e uma partição de flash reservada para NVS no devicetree — configuração adicional no overlay da Nucleo.
- A flash tem um número finito de ciclos de escrita (~10.000–100.000 por setor dependendo do processo). Para o padrão de uso deste projeto (threshold alterado esporadicamente) isso não é relevante.
- `settings_load()` no boot adiciona latência de alguns milissegundos antes das threads iniciarem.

## Alternativas consideradas

| Alternativa | Motivo de rejeição |
|-------------|-------------------|
| Escrita direta em flash via `flash_write()` | Sem abstração de desgaste, sem tolerância a falhas de energia, muito mais código |
| EEPROM externa | Hardware adicional desnecessário — a flash interna é suficiente |
| Hardcode em Kconfig apenas | Threshold não pode ser alterado sem recompilar — derrota o propósito do shell |
| Settings com backend filesystem (littlefs) | Requer mais flash e RAM; overkill para um único parâmetro |
