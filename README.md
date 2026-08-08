# 💧 ESP32 - Monitor de Nível de Poço & Reservatório

Sistema inteligente e autônomo de telemetria e monitoramento de nível de água para poços e caixas d'água utilizando o microcontrolador **ESP32**, sensor ultrassônico com filtragem de ruídos, amostragem dinâmica adaptativa, sistema de contingência offline em Flash (**LittleFS**), atualizações sem fio (**ArduinoOTA**), dashboard web embarcado em tempo real e integração com banco de dados serverless em nuvem (**Turso / SQLite Edge**).

![ESP32](https://img.shields.io/badge/Microcontrolador-ESP32-blue?logo=espressif)
![PlatformIO](https://img.shields.io/badge/Framework-Arduino_PlatformIO-orange?logo=platformio)
![LittleFS](https://img.shields.io/badge/Storage-LittleFS_Flash-green)
![ArduinoOTA](https://img.shields.io/badge/OTA-Wireless_Update-red)
![Chart.js](https://img.shields.io/badge/Frontend-Chart.js-ff6384?logo=chartdotjs)
![Turso](https://img.shields.io/badge/Database-Turso_SQLite-4433ff?logo=sqlite)
[![GitHub Repository](https://img.shields.io/badge/GitHub-ESP32__monitor__poco-181717?logo=github)](https://github.com/olafotte/ESP32_monitor_poco)

> 🔗 **Repositório oficial no GitHub:** [https://github.com/olafotte/ESP32_monitor_poco](https://github.com/olafotte/ESP32_monitor_poco)

---

## 🌟 Funcionalidades Principais

* **📏 Medição Ultrassônica com Filtragem de Ressonância**: Realiza rajadas de 5 leituras rápidas por ciclo e filtra ecos e variações espúrias provocadas por paredes rugosas de poços.
* **⚡ Modo Dinâmico Adaptativo (Detecção de Chuva e Consumo)**:
  * Analisa continuamente 2 janelas deslizantes de medições (janela recente de 1 minuto vs. janela passada de 1 a 2 minutos).
  * Ao detectar variação súbita de nível (subida por enxurrada/chuva ou descida rápida por bombeamento/consumo), acelera automaticamente o intervalo de envio para a nuvem de **5 minutos (Modo Seco)** para **1 minuto (Modo Chuva/Alerta)**.
  * Conta com tempo de *Cooldown* configurável (padrão 30 minutos) antes de retornar ao consumo reduzido.
* **💾 Armazenamento Contingencial Offline (LittleFS)**:
  * Se a conexão Wi-Fi ou com a nuvem Turso falhar, as medições com *timestamp* Unix são salvas no sistema de arquivos Flash interno (`/offline_queue.dat`).
  * Assim que a rede se restabelece, os dados pendentes são descarregados automaticamente em lotes (*batches* de 20 registros) para o Turso sem perda de histórico.
* **📊 Telemetria Deslizante de 7 Dias**: Mantém um histórico de enquadramento móvel gravado na NVS (Preferences) contabilizando o total de envios com sucesso para a nuvem e registros salvos offline.
* **📶 Portal Captivo Wi-Fi & mDNS**:
  * Ao ligar sem Wi-Fi configurado, cria o Ponto de Acesso `Config_Sensor_Poco` com servidor DNS captivo (`192.168.4.1`).
  * Permite fácil acesso à Dashboard Local pelo nome de rede **`http://monitorpoco.local`**.
* **📡 Atualizações Sem Fio (ArduinoOTA)**: Suporte nativo a atualizações de firmware via Wi-Fi sem a necessidade de conectar o ESP32 ao computador via cabo USB.
* **🔊 Sinalização Sonora & Efeitos R2-D2**:
  * Emite avisos visuais (LED GPIO 2) e sonoros (Buzzer GPIO 27) com modulação de frequência (estilo R2-D2) para boot, conexão Wi-Fi, sucesso de envio ao Turso, gravação offline e erros.
  * Permite ativar ou desativar o *beep* a cada leitura ultrassônica diretamente pelo Web Dashboard.
* **🖥️ Dashboard Web Embarcado (Dark Mode)**:
  * Interface moderna e responsiva gravada em `PROGMEM` (`src/index.h`).
  * Visualização dinâmica com gráfico em tempo real (**Chart.js**), barra de reservatório animada %, cartões de métricas e painel interativo de configurações.

---

## 🛠️ Esquema de Hardware (Pinout)

| Componente | Pino no ESP32 | Função |
| :--- | :--- | :--- |
| **Sensor Ultrassônico (Trig)** | `GPIO 25` | Disparo de pulso ultrassônico |
| **Sensor Ultrassônico (Echo)** | `GPIO 26` | Leitura do tempo de retorno do eco |
| **LED Indicador** | `GPIO 2` | Sinalização visual de status e ciclos |
| **Buzzer Piezoelétrico** | `GPIO 27` | Alarme sonoro e efeitos sonoros de diagnóstico |
| **Botão Reset Wi-Fi / NVS** | `GPIO 14` | Reset físico para padrão de fábrica (Pull-up interno) |

---

## 📁 Estrutura do Projeto

```text
ESP32_monitor_poco/
├── platformio.ini         # Configurações do PlatformIO, dependências e perfis (Cable / OTA)
├── src/
│   ├── index.h            # Interface Web Dashboard (HTML5, CSS3, JS e Chart.js em PROGMEM)
│   ├── secrets.h          # Credenciais privadas de acesso ao Turso (Ignorado pelo Git)
│   ├── secrets.h.example  # Modelo de credenciais para novos ambientes
│   └── main.cpp           # Código principal (Wi-Fi, LittleFS, Turso Hrana v2, OTA, Modo Dinâmico)
├── .gitignore             # Regras de exclusão do Git (protege secrets.h e .pio)
└── README.md              # Documentação oficial do projeto
```

---

## 🔒 Credenciais e Segurança (`secrets.h`)

Por razões de segurança, o arquivo `src/secrets.h` (que contém o Token de Autorização Bearer e a URL da API do Turso) **não é enviado ao repositório público**.

Para configurar um novo ambiente:
1. Copie o arquivo modelo `src/secrets.h.example` para `src/secrets.h`:
   ```bash
   cp src/secrets.h.example src/secrets.h
   ```
2. Abra `src/secrets.h` e preencha as variáveis com suas credenciais do banco Turso:
   ```cpp
   #define DEFAULT_TURSO_URL "https://seu-banco-turso.turso.io/v2/pipeline"
   #define DEFAULT_TURSO_TOKEN "seu_token_jwt_aqui"
   ```

---

## 🚀 Como Compilar e Gravar Firmware

### Pré-requisitos
* [VS Code](https://code.visualstudio.com/) com a extensão **PlatformIO IDE** instalada.
* Placa ESP32 (Dev Module).

### 1. Gravação via Cabo USB (`esp32dev_cable`)
1. Conecte o ESP32 ao computador via cabo USB.
2. Certifique-se de que criou e preencheu o arquivo `src/secrets.h`.
3. Abra o terminal do PlatformIO no VS Code e execute:
   ```bash
   pio run -e esp32dev_cable -t upload
   ```
4. Para visualizar as mensagens de diagnóstico no Monitor Serial (115200 baud):
   ```bash
   pio device monitor
   ```

### 2. Gravação Sem Fio via Wi-Fi (`esp32dev_ota`)
Após gravar o firmware via cabo pela primeira vez e conectar o ESP32 à sua rede Wi-Fi local:
1. O ESP32 estará visível na rede como `monitorpoco.local` na porta OTA `3232`.
2. Para enviar novas atualizações via Wi-Fi, execute:
   ```bash
   pio run -e esp32dev_ota -t upload
   ```

---

## ⚙️ Primeiro Acesso e Configuração

1. Se não houver dados de rede salvos, o ESP32 iniciará em modo **Access Point**.
2. Conecte seu smartphone ou computador à rede Wi-Fi **`Config_Sensor_Poco`**.
3. O portal captivo abrirá automaticamente. Caso não abra, acesse `http://192.168.4.1`.
4. Digite as credenciais da sua rede Wi-Fi, a **Profundidade Vazia** (distância em cm do sensor ao fundo) e a **Profundidade Cheia** (distância em cm do sensor à água cheia).
5. Salve as configurações. O ESP32 reiniciará e se conectará à sua rede doméstica.
6. Abra o navegador e acesse a Dashboard local: **`http://monitorpoco.local`**.

---

## 🔌 API REST Embarcada (JSON)

O ESP32 fornece endpoints HTTP para integração e leitura dos dados em tempo real:

| Método | Endpoint | Descrição |
| :--- | :--- | :--- |
| `GET` | `/` | Retorna o Web Dashboard HTML5. |
| `GET` | `/dados` | Retorna o estado atual da telemetria em formato JSON (nível %, distância cm, pendências LittleFS, status Turso, amostragem ativa, contadores 7 dias). |
| `POST` | `/config_beep` | Habilita ou desabilita a sinalização sonora por leitura (`enabled=true/false`). |
| `POST` | `/config_parametros` | Atualiza parâmetros de profVazia, profCheia, intervalo de amostragem seco/chuva e limiar de acionamento dinâmico. |
| `POST` | `/zerar_telemetria` | Reseta os contadores de 7 dias e limpa a fila de contingência LittleFS. |
| `POST` | `/simular_falha` | Alterna o modo de teste para simular quedas de conexão Wi-Fi/Nuvem. |

---

## 💡 Restauração de Padrões de Fábrica (Reset)

Para apagar o SSID, a senha e todas as configurações salvas na memória NVS (Preferences):
1. Com o ESP32 desligado, pressione e segure o **botão conectado ao GPIO 14**.
2. Ligue ou reinicie o ESP32 mantendo o botão pressionado por cerca de 3 segundos.
3. O sistema emitirá um tom sonoro longo, apagará os dados salvos e iniciará novamente o Portal Captivo `Config_Sensor_Poco`.

