# Dicionário do Leigo — SafeGuard Node

Este glossário explica os termos técnicos do projeto em linguagem acessível a quem não é da área de sistemas embarcados.

---

## A

**ADXL345**
Um chip de tamanho milimétrico que mede aceleração nos três eixos do espaço (cima/baixo, esquerda/direita, frente/atrás). É o "sensor de movimento" do projeto — o mesmo tipo de sensor que existe em smartphones para saber se você virou a tela.

**Alarme**
Estado do sistema após detectar um impacto ou movimentação acima do limite configurado. Enquanto em alarme, o LED pisca rapidamente (5 vezes por segundo). O sistema permanece em alarme até que alguém o desarme explicitamente — ele não esquece sozinho.

**Armar**
Ativar a vigilância do dispositivo. Quando armado, o sistema registra a posição atual como referência e começa a comparar cada nova leitura com ela. Qualquer desvio grande gera um alarme.

---

## B

**Bare-metal**
Programar o hardware diretamente, sem usar camadas prontas de software para fazer o trabalho. É como dirigir um carro com câmbio manual e pedal de embreagem em vez de câmbio automático. Dá mais controle e visibilidade sobre o que acontece no nível elétrico.

**Baseline**
A "foto" da posição do objeto no momento do arme. O sistema usa essa foto como ponto de comparação para decidir se algo se moveu.

**Botão long press**
Pressionar e segurar o botão físico por mais de 1,5 segundos. É um mecanismo de segurança para evitar armar/desarmar por acidente com uma pressão rápida.

---

## C

**Chip Select (CS)**
Um sinal elétrico (um fio) que o microcontrolador usa para "chamar" um sensor específico no barramento SPI. Funciona como um "você, estou falando com você" — sem isso, vários sensores no mesmo barramento ficariam respondendo ao mesmo tempo e causariam conflito.

**CONFIG_ADXL345**
Uma opção de compilação do Zephyr que, quando ativada, usa o driver pronto de software para o sensor ADXL345. Neste projeto ela é **intencionalmente desativada** para que o protocolo de comunicação seja implementado à mão, tornando o tráfego visível e educativo.

**Cortex-M4**
A arquitetura do processador dentro do microcontrolador STM32F401. É um processador de 32 bits fabricado pela ARM, comum em dispositivos embarcados. O "M4" indica que tem unidade de ponto flutuante em hardware (mas o projeto não a usa intencionalmente).

---

## D

**Delta**
A diferença entre a leitura atual do sensor e o baseline. Matematicamente é a magnitude do vetor de diferença nos três eixos. Um delta grande significa que o objeto se moveu muito.

**Desarmar**
Desativar a vigilância. O LED apaga e o sistema para de verificar movimentos. Se estava em alarme, o desarme também reconhece e encerra o alarme.

**Devicetree**
Um arquivo de texto que descreve o hardware do sistema para o Zephyr em tempo de compilação. Funciona como uma "planta baixa" do circuito — indica quais pinos fazem o quê, quais periféricos existem e como estão conectados.

---

## E

**Evento**
Cada vez que o sistema detecta um impacto e entra em alarme conta como um evento. O contador de eventos acumula desde o último arme e pode ser consultado pelo shell.

---

## F

**Flash**
Memória não volátil dentro do microcontrolador — equivalente a um SSD minúsculo. O firmware (o programa) fica gravado na flash e sobrevive a quedas de energia. O projeto também usa uma área especial da flash (NVS) para guardar configurações do usuário.

**FSM (Máquina de Estados Finitos)**
Uma forma de modelar o comportamento de um sistema como um conjunto de estados possíveis e regras de transição entre eles. Neste projeto: DESARMADO → ARMADO → ALARME → DESARMADO. O sistema sempre sabe em qual estado está e o que pode acontecer a partir dele.

---

## G

**GPIO**
Pino de propósito geral do microcontrolador que pode funcionar como entrada (leitura de botão) ou saída (acender LED). É o meio mais simples de comunicação elétrica — apenas ligado ou desligado.

---

## I

**ISR (Rotina de Serviço de Interrupção)**
Uma função especial que o processador executa imediatamente quando um evento externo ocorre (por exemplo, o botão sendo pressionado), interrompendo o que estava fazendo. ISRs devem ser curtíssimas — qualquer operação demorada dentro de uma ISR pode travar o sistema inteiro.

---

## K

**Kconfig**
Sistema de configuração do Zephyr. Funciona como um painel de controle em tempo de compilação — você ativa ou desativa subsistemas inteiros (SPI, logging, shell, NVS) e define valores padrão. As opções ficam no arquivo `prj.conf`.

---

## L

**LED LD2**
O LED verde que já vem soldado na placa Nucleo-F401RE, conectado ao pino PA5. Não precisa de hardware externo — o projeto usa ele para sinalizar os estados: apagado (desarmado), fixo (armado), piscando 5 Hz (alarme).

**LSB (Least Significant Bit)**
A menor unidade de medida digital de um sensor. Para o ADXL345 em modo full-resolution ±16g, 1 LSB equivale a ~3,9 mg (miligraças). O threshold padrão de 80 LSB representa aproximadamente 0,3 g — o suficiente para detectar uma pancada na bancada mas ignorar vibração ambiente.

**Long press**
Veja "Botão long press".

---

## M

**Microcontrolador (MCU)**
Um computador completo em um único chip: processador, memória e periféricos de comunicação (SPI, UART, GPIO). O STM32F401 usado neste projeto tem 512 KB de flash e 96 KB de RAM — suficiente para rodar o Zephyr com todas as features do projeto.

---

## N

**NVS (Non-Volatile Storage)**
Área reservada da flash do microcontrolador gerenciada pelo subsistema Settings do Zephyr. Permite salvar pares chave-valor que sobrevivem a reinicializações. O projeto usa `sg/threshold` como chave para o threshold configurado pelo usuário.

**Nucleo-F401RE**
A placa de desenvolvimento usada no projeto. Fabricada pela STMicroelectronics, contém o microcontrolador STM32F401 mais um chip auxiliar (ST-LINK) que permite gravar o firmware e monitorar o console serial pelo mesmo cabo USB.

---

## O

**Overlay (.overlay)**
Arquivo que estende o devicetree de uma placa específica para uma aplicação. No projeto, o overlay adiciona o nó do ADXL345 ao barramento SPI1 da Nucleo — diz ao Zephyr que existe um sensor conectado nos pinos D10–D13.

---

## P

**Publish-Subscribe (pub-sub)**
Padrão de comunicação onde quem produz informação (publisher) não sabe quem vai consumir, e vice-versa. O ZBus implementa esse padrão: `sensor_thread` publica leituras sem saber que `proc_thread` vai processar, e `proc_thread` publica estado sem saber que `led_thread` vai acender o LED.

---

## Q

**QEMU**
Um emulador de hardware que roda no computador e simula um microcontrolador ARM. Permite compilar e testar o firmware sem ter a placa física. Neste projeto é usado para desenvolvimento antes de ter o hardware e para rodar os testes automatizados.

---

## R

**RTOS (Sistema Operacional de Tempo Real)**
Sistema operacional projetado para garantir que tarefas críticas sejam executadas dentro de prazos precisos. Diferente de um sistema operacional de desktop, o RTOS decide qual tarefa roda a cada microssegundo com base em prioridades. O Zephyr é o RTOS deste projeto.

---

## S

**SPI (Serial Peripheral Interface)**
Protocolo de comunicação serial entre o microcontrolador e periféricos (como o ADXL345). Usa 4 fios: clock (SCK), dados de saída (MOSI), dados de entrada (MISO) e seleção do chip (CS). É síncrono — o clock pulsa e dados são transferidos bit a bit.

**Shell UART**
Interface de linha de comando acessível pela porta serial. Permite que o operador envie comandos ao dispositivo via terminal (como `safeguard arm`, `safeguard threshold 120`) sem precisar recompilar o firmware.

**ST-LINK**
Chip auxiliar presente na placa Nucleo que faz a ponte entre o USB do computador e o microcontrolador principal. Permite gravar firmware, depurar com GDB e usar o console serial — tudo pelo mesmo cabo USB.

---

## T

**Thread**
Uma tarefa independente com sua própria pilha de execução, gerenciada pelo kernel do RTOS. No projeto, cada responsabilidade tem sua própria thread: leitura do sensor, lógica de detecção, controle do LED. O scheduler decide qual thread roda a cada momento.

**Threshold**
O limiar de sensibilidade do detector de impacto, em LSB. Se o delta calculado ultrapassar esse valor, o alarme dispara. Pode ser ajustado pelo operador via shell e é persistido em NVS.

---

## U

**UART**
Universal Asynchronous Receiver-Transmitter — protocolo de comunicação serial usado para o console e o shell. Na Nucleo-F401RE, o UART2 está conectado ao ST-LINK, que o expõe como porta serial virtual no computador (geralmente `/dev/tty.usbmodem*` no macOS).

---

## Z

**ZBus**
Subsistema de mensagens do Zephyr baseado em canais tipados. Permite que threads se comuniquem sem acoplamento direto. Uma thread publica uma mensagem no canal; todas as threads inscritas naquele canal são notificadas. No projeto há 3 canais: `accel_data`, `system_state` e `arm_event`.

**Zephyr RTOS**
O sistema operacional de tempo real open-source criado pela Linux Foundation. É o coração do projeto — fornece o scheduler de threads, os drivers de SPI/GPIO/UART, o subsistema ZBus, o shell, o NVS e a infraestrutura de testes (ZTEST).

**ZTEST**
Framework de testes unitários do Zephyr. Permite escrever e rodar casos de teste que verificam funções individuais (como `sg_detect_impact()`) sem precisar do hardware real — os testes rodam em emulação QEMU.
