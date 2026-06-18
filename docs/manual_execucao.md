# Manual de Execução — SafeGuard Node

---

## Pré-requisitos

| Ferramenta | Versão mínima | Verificar |
|------------|---------------|-----------|
| Python | 3.12 | `python3 --version` |
| CMake | 3.20 | `cmake --version` |
| Ninja | qualquer | `ninja --version` |
| QEMU ARM | qualquer | `qemu-system-arm --version` |
| Zephyr SDK | 0.17.4 | diretório `~/zephyr-sdk-0.17.4` |
| dtc | 1.4.6 | `dtc --version` |

Instalar no macOS via Homebrew:
```sh
brew install cmake ninja dtc qemu
```

---

## Configurar o ambiente

Execute uma vez por sessão de terminal, a partir da raiz do workspace (`/Users/khrir/develop/zephyr`):

```sh
source .venv/bin/activate        # ativa o Python com o west instalado
source zephyr/zephyr-env.sh      # define ZEPHYR_BASE e ajusta PATH
```

Verifique se o ambiente está correto:
```sh
west --version                   # deve mostrar West version: v1.5.0 (ou superior)
echo $ZEPHYR_BASE                # deve mostrar o caminho para zephyr/
```

> **Atenção:** Se o `.venv` foi criado em outro caminho e `west` falha, recrie o ambiente:
> ```sh
> rm -rf .venv
> python3 -m venv .venv
> source .venv/bin/activate
> pip install -r zephyr/scripts/requirements-base.txt
> ```

---

## Build

### Emulação (sem hardware) — macOS

`native_sim` não é suportado no macOS. Use `mps2/an385` (Cortex-M3 via QEMU):

```sh
# Primeiro build (ou após mudar board)
west build -b mps2/an385 safeguard-node --build-dir safeguard-node/build

# Rebuild rápido (só recompila o que mudou)
west build --build-dir safeguard-node/build

# Pristine — obrigatório após mudar prj.conf, .overlay ou board
west build -b mps2/an385 safeguard-node --build-dir safeguard-node/build -p always
```

### Hardware real — Nucleo-F401RE

```sh
west build -b nucleo_f401re safeguard-node --build-dir safeguard-node/build
west build -b nucleo_f401re safeguard-node --build-dir safeguard-node/build -p always
```

---

## Executar

### No QEMU (emulação)

```sh
west build --build-dir safeguard-node/build -t run
```

Saída esperada:
```
*** Booting Zephyr OS build v4.3.0-xxxx ***
[00:00:00.000,000] <inf> safeguard: SafeGuard Node iniciando...
[00:00:00.000,000] <inf> safeguard: rodando
```

**Para sair do QEMU:** `Ctrl+A` depois `X`

> O QEMU emula o tempo mais rápido que o relógio de parede — os timestamps no log estão corretos, mas o tempo real decorrido será menor que o indicado.

### Na placa real

Conecte a Nucleo via USB (ST-LINK), então:

```sh
west flash                       # grava o firmware via ST-LINK
```

Abra o console serial para ver os logs:
```sh
screen /dev/tty.usbmodem* 115200
# Sair: Ctrl+A → K → Y
```

Alternativa com minicom:
```sh
minicom -D /dev/tty.usbmodem* -b 115200
# Sair: Ctrl+A → Q
```

---

## Shell UART

Com o console serial aberto, o shell do Zephyr responde ao prefixo `safeguard`:

```sh
uart:~$ safeguard status
Estado: DESARMADO | Threshold: 80 LSB | Eventos: 0

uart:~$ safeguard arm
[00:00:05.000] <inf> safeguard: sistema armado (baseline capturado)

uart:~$ safeguard threshold 120
Threshold atualizado: 120 LSB (salvo em NVS)

uart:~$ safeguard disarm
[00:00:10.000] <inf> safeguard: sistema desarmado

uart:~$ safeguard reset
Configuração restaurada para padrão de fábrica
```

| Comando | Efeito |
|---------|--------|
| `safeguard status` | Exibe estado atual, threshold e contagem de eventos |
| `safeguard arm` | Arma o sistema (equivale ao botão long press) |
| `safeguard disarm` | Desarma ou reconhece um alarme |
| `safeguard threshold <n>` | Define novo threshold em LSB (intervalo: 10–1000) |
| `safeguard reset` | Restaura threshold padrão e zera event_count |

---

## Botão físico (USER button — PC13)

| Ação | Resultado |
|------|-----------|
| Pressão longa (>1,5 s) em DESARMADO | → ARMADO |
| Pressão longa (>1,5 s) em ARMADO | → DESARMADO |
| Pressão longa (>1,5 s) em ALARME | → DESARMADO (reconhece) |
| Pressão curta | ignorada |

---

## Testes automatizados

Os testes rodam em `qemu_cortex_m3` — não precisam de hardware real.

```sh
# Roda toda a suite de testes
west twister -T safeguard-node/tests -p qemu_cortex_m3 --inline-logs

# Roda um caso específico
west twister -T safeguard-node/tests -p qemu_cortex_m3 -s safeguard.test_detect
```

Saída esperada (todos passando):
```
Running tests...
PASSED - safeguard.test_no_impact_below_threshold
PASSED - safeguard.test_impact_above_threshold
PASSED - safeguard.test_fsm_arm_transition
PASSED - safeguard.test_fsm_alarm_on_impact
PASSED - safeguard.test_settings_round_trip
```

---

## Referência rápida de build

```sh
# Ambiente (rodar sempre que abrir terminal novo)
cd /Users/khrir/develop/zephyr
source .venv/bin/activate
source zephyr/zephyr-env.sh

# Emulação
west build -b mps2/an385 safeguard-node --build-dir safeguard-node/build
west build --build-dir safeguard-node/build -t run          # QEMU: sair Ctrl+A → X

# Hardware real
west build -b nucleo_f401re safeguard-node --build-dir safeguard-node/build -p always
west flash

# Testes
west twister -T safeguard-node/tests -p qemu_cortex_m3 --inline-logs

# Inspecionar configuração Kconfig do build atual
west build --build-dir safeguard-node/build -t menuconfig
```

---

## Solução de problemas

**`west: command not found`**
O venv não está ativo. Execute `source .venv/bin/activate` da raiz do workspace.

**`ZEPHYR_BASE not set`**
Execute `source zephyr/zephyr-env.sh` da raiz do workspace.

**`Build directory is for application X, but Y was specified`**
O diretório de build tem cache de outro projeto. Use `-p always` para limpar:
```sh
west build -b mps2/an385 safeguard-node --build-dir safeguard-node/build -p always
```

**`Cannot find module 'pptxgenjs'`** (ao gerar slides)
```sh
node gen_slides.js  # deve ser executado dentro de docs/
```

**`spi_is_ready_dt` retorna false na placa real**
Verifique se o overlay está sendo aplicado (board correta no build) e se os pinos estão fisicamente conectados.

**QEMU trava sem imprimir nada**
Aguarde 2–3 segundos — o boot no QEMU demora mais que na placa real. Se nada aparecer, verifique se o build foi para `mps2/an385` e não para `nucleo_f401re` (que não tem suporte QEMU).
