# VoiceRecognitionV3_ESP — Biblioteca Arduino para o módulo Voice Recognition V3.1 (ESP32)

Biblioteca Arduino voltada ao **módulo Elechouse Voice Recognition V3.1**, adaptada para
**ESP32** e comunicação **UART** via `HardwareSerial` (ex.: `Serial2`).

A ideia é oferecer o driver de baixo nível do protocolo do módulo, mantendo a API pública
estável para uso em projetos existentes.

## 1) Visão geral

O Voice Recognition V3.1 é um módulo que **reconhece comandos de voz previamente treinados**
no próprio hardware. O fluxo básico é:

1. **Train**: o usuário grava um comando em um **record ID**.
2. **Load**: carrega um ou mais `record IDs` no reconhecedor.
3. **Recognize**: o módulo escuta e retorna qual ID foi reconhecido.

Esta biblioteca implementa o protocolo UART oficial e expõe métodos simples para essas
operações no ESP32.

## 2) Instalação

### Arduino IDE (ZIP)
1. Baixe este repositório como **ZIP**.
2. Arduino IDE: **Sketch → Include Library → Add .ZIP Library...**
3. Reinicie a IDE.

### Instalação manual
Copie a pasta deste repositório para:

```
<Arduino>/libraries/VoiceRecognitionV3_ESP
```

No seu sketch:

```cpp
#include <VoiceRecognitionV3_ESP.h>
```

## 3) Wiring (ESP32)

Exemplo comum usando `Serial2`:

```
ESP32 TX (GPIO17)  ->  VR3.1 RX
ESP32 RX (GPIO16)  <-  VR3.1 TX
GND               ->  GND
```

**Observações:**
- `RX/TX` podem ser alterados via `begin(...)`.
- Garanta **GND comum** entre ESP32 e o módulo.
- Alimentação e nível lógico devem seguir o seu módulo/placa específica.

## 4) Quick Start

1. Abra `examples/vr_sample_train/vr_sample_train.ino`.
2. Configure `RX_PIN` e `TX_PIN` se necessário.
3. Faça upload para o ESP32.
4. No **Serial Monitor (115200)**, digite um número de record (ex.: `1`) e pressione Enter.
5. Fale o comando quando o módulo solicitar.
6. Em seguida, rode `examples/vr_sample_multi_cmd` para reconhecer.

## 5) Guia de treino e reconhecimento

- **train()**: grava um comando em um ou mais `record IDs`. O módulo solicita a fala e
  devolve o status de treinamento.
- **load()**: carrega os `record IDs` treinados na memória do reconhecedor.
- **recognize()**: espera um pacote com o `record ID` reconhecido.

Fluxo recomendado:
1. `train([id])`
2. `load([id])`
3. `recognize()` (em loop)

## 6) API (resumo)

| Método | Descrição | Retorno | Observações |
| --- | --- | --- | --- |
| `begin(baud, rx, tx)` | Inicializa UART | `void` | Padrão: 9600, RX=16, TX=17 |
| `train(records, count, outBuf, outLen)` | Treina comandos | `>=0` bytes / `<0` erro | `outBuf` recebe status do treino |
| `load(records, count, outBuf, outLen)` | Carrega IDs no recognizer | `>=0` bytes / `<0` erro | `outBuf` com status de carga |
| `clear()` | Limpa comandos carregados | `0` ok / `<0` erro | Apenas recognizer |
| `recognize(outBuf, outLen, timeout)` | Aguarda reconhecimento | `>=0` bytes / `<0` erro | `outBuf[1]` é o record reconhecido |
| `checkRecognizer(outBuf, outLen, timeout)` | Status do recognizer | `>=0` bytes / `<0` erro | Verifica buffer de reconhecimento |

### Códigos de erro comuns
- **-1**: parâmetros inválidos ou buffer pequeno.
- **-2**: timeout / pacote incompleto.
- **-3**: comando inesperado (resposta não esperada).
- **-4**: `outBuf` insuficiente para a resposta.

## 7) Troubleshooting

- **Nada aparece no Serial Monitor**
  - Confirme `Serial.begin(115200)` e a taxa do monitor.
  - Aguarde 1–2s após reset para o monitor conectar.

- **Não reconhece comandos**
  - Treine o `record ID` primeiro.
  - Garanta que o ID treinado foi carregado (`load`).
  - Fale em ambiente mais silencioso.

- **Timeouts frequentes**
  - Verifique se TX/RX estão corretos (pode precisar inverter).
  - Confirme baud rate do módulo (padrão 9600).

- **Resposta incoerente**
  - Verifique se o módulo está corretamente alimentado.
  - Teste o exemplo `vr_sample_bridge` para diagnosticar comunicação UART.

## 8) Compatibilidade

- **ESP32** (Arduino core oficial)
- Arduino IDE 1.8+ ou 2.x
- Não há dependências externas além do core Arduino

## 9) Créditos

- Baseado no projeto Elechouse VoiceRecognitionV3:
  https://github.com/elechouse/VoiceRecognitionV3

Esta biblioteca é uma adaptação para ESP32 e usa a mesma ideia de protocolo, com
API própria preservada para projetos existentes.
