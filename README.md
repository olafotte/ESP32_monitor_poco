# 💧 ESP32 - Monitor de Nível de Poço & Caixa d'Água

Sistema completo de telemetria e monitoramento do nível de água para poços e caixas d'água utilizando o microcontrolador **ESP32**, sensor ultrassônico, dashboard web em tempo real e integração com banco de dados em nuvem (**Turso / SQLite Edge**).

![ESP32](https://img.shields.io/badge/Microcontrolador-ESP32-blue)
![PlatformIO](https://img.shields.io/badge/Framework-Arduino_PlatformIO-orange)
![Chart.js](https://img.shields.io/badge/Frontend-Chart.js-ff6384)
![Turso](https://img.shields.io/badge/Database-Turso_SQLite-4433ff)
[![GitHub Repository](https://img.shields.io/badge/GitHub-ESP32__monitor__poco-181717?logo=github)](https://github.com/olafotte/ESP32_monitor_poco)

> 🔗 **Repositório oficial no GitHub:** [https://github.com/olafotte/ESP32_monitor_poco](https://github.com/olafotte/ESP32_monitor_poco)

---

## 🌟 Funcionalidades Principais

* **📏 Medição Ultrassônica com Filtragem de Ressonância**: Realiza 5 leituras rápidas e seleciona a maior distância válida para desconsiderar ecos ou ruídos nas paredes do poço.
* **📊 Dashboard Web Local (Dark Mode)**: Servidor web embarcado acessível via `http://monitorpoco.local` (mDNS) com gráficos dinâmicos em **Chart.js**, cartões de estatísticas e visualizador de barra de nível (%).
* **📱 Portal Captivo Wi-Fi (DNS Server)**: Cria o Ponto de Acesso `Config_Sensor_Poco` se não houver rede salva. Redireciona o celular automaticamente para a página de configuração ao conectar.
* **💾 Armazenamento NVS (Preferences)**: Salva SSID, Senha, Token da Nuvem e dimensões do poço (profundidade cheia/vazia em cm) na memória não-volátil.
* **☁️ Integração com Banco de Dados Nuvem (Turso)**: Transmissão HTTPS assíncrona (protocolo Hrana v2) dos dados consolidados a cada 5 minutos.
* **🔒 Separação de Credenciais Seguras**: Mantém tokens de acesso em arquivo privado `src/secrets.h` ignorado pelo Git.
* **⚡ Reconexão Wi-Fi Não-Bloqueante**: O sistema continua efetuando leituras locais e tentando reconectar sem travar a execução ou reiniciar o chip.

---

## 🛠️ Esquema de Hardware (Pinout)

| Componente | Pino no ESP32 | Função |
| :--- | :--- | :--- |
| **Sensor Ultrassônico (Trig)** | `GPIO 25` | Emite o pulso ultrassônico |
| **Sensor Ultrassônico (Echo)** | `GPIO 26` | Recebe o eco do sinal refletido |
| **LED Indicador** | `GPIO 2` | Sinalização de status e rajada de leitura |
| **Buzzer** | `GPIO 27` | Alarme sonoro local de emergência |
| **Botão de Reset Wi-Fi** | `GPIO 14` | Reset físico das configurações (Pull-up interno) |

---

## 📁 Estrutura do Projeto

```text
ESP32_monitor_poco/
├── platformio.ini         # Configurações do PlatformIO e dependências (ArduinoJson)
├── src/
│   ├── index.h            # Interface Web (HTML5/CSS3/JS + Chart.js em PROGMEM)
│   ├── secrets.h          # Credenciais privadas (Turso Token/URL - Ignorado no Git)
│   ├── secrets.h.example  # Modelo de credenciais para novos ambientes
│   └── main.cpp           # Firmware C++ (Wi-Fi, DNS, Sensor, HTTP Client, Turso)
└── README.md              # Documentação do projeto
```

---

## 🔒 Credenciais e Segurança (`secrets.h`)

Por motivos de segurança, o arquivo `src/secrets.h` (que contém os tokens de acesso ao banco Turso) **não é enviado ao repositório público no GitHub** (está no `.gitignore`).

Para novos ambientes ou novos desenvolvedores:
1. Copie o arquivo modelo `src/secrets.h.example` para `src/secrets.h`:
   ```bash
   cp src/secrets.h.example src/secrets.h
   ```
2. Insira suas credenciais e tokens reais do Turso no arquivo `src/secrets.h`.

---

## 🚀 Como Compilar e Gravar

### Pré-requisitos
* [VS Code](https://code.visualstudio.com/) com a extensão **PlatformIO IDE** instalada.
* ESP32 conectado via USB no seu computador.

### Passos para Gravação
1. Clone ou abra este repositório no VS Code / PlatformIO.
2. Certifique-se de ter criado o arquivo `src/secrets.h` conforme o modelo.
3. Verifique a porta COM configurada no arquivo `platformio.ini`:
   ```ini
   upload_port = COM6
   monitor_port = COM6
   ```
4. Compilar o projeto:
   ```bash
   pio run
   ```
5. Gravar o firmware no ESP32:
   ```bash
   pio run -t upload
   ```
6. Abrir o Monitor Serial (115200 baud):
   ```bash
   pio device monitor
   ```

---

## ⚙️ Primeiro Acesso e Configuração

1. Se não houver rede Wi-Fi configurada, o ESP32 iniciará no modo **Ponto de Acesso**.
2. No seu celular ou computador, conecte-se ao Wi-Fi chamado `Config_Sensor_Poco`.
3. O portal captivo abrirá automaticamente. Caso não abra, acesse `http://192.168.4.1` no navegador.
4. Preencha as credenciais do Wi-Fi do local, a **Profundidade Vazia** (fundo do poço em cm) e a **Profundidade Cheia** (distância até o nível máximo de água).
5. Clique em **Salvar e Conectar**. O ESP32 reiniciará e se conectará à rede.
6. Acesse a dashboard local pelo endereço: **`http://monitorpoco.local`**.

---

## 💡 Restaurar Padrões de Fábrica (Reset)
Caso queira alterar a rede Wi-Fi ou apagar as configurações salvas, segure o **Botão conectado ao GPIO 14** no momento em que o ESP32 for ligado ou reiniciado. As configurações salvas na NVS serão limpas e o portal de configuração abrirá novamente.
