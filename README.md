
# Data Logger IoT — Monitoramento Ambiental

**Projeto acadêmico de Sistemas Embarcados**  
Engenharia de Computação — 5º Semestre

---

## 1. Apresentação e objetivo

O Data Logger IoT é um sistema embarcado desenvolvido para
monitorar, registrar e analisar condições ambientais por meio
de sensores de temperatura, umidade e luminosidade.

O projeto integra hardware, firmware, comunicação MQTT,
computação em nuvem e desenvolvimento web, aplicando conceitos
de Internet das Coisas (IoT).

O sistema utiliza um ESP32 para aquisição de dados e uma
infraestrutura FIWARE hospedada em uma instância EC2 do
AWS Learner Lab.

A aplicação web, desenvolvida em ASP.NET Core MVC, permite:

- Monitoramento de variáveis ambientais;
- Cadastro e gerenciamento de dispositivos;
- Consulta de dados históricos;
- Visualização de gráficos;
- Cálculo de médias das medições;
- Identificação de condições fora dos limites;
- Envio de comandos para o dispositivo físico;
- Acionamento de alertas visuais e sonoros.

---

## 2. Arquitetura do sistema

![Arquitetura do Data Logger IoT](docs/arquitetura.svg)

A arquitetura do projeto é composta pelas seguintes camadas:

| Camada | Tecnologia | Responsabilidade |
|---|---|---|
| Hardware | ESP32 e sensores | Aquisição de dados ambientais |
| Firmware | C++ / Arduino IDE | Processamento e comunicação |
| Comunicação | MQTT | Transmissão de dados e comandos |
| Infraestrutura | AWS EC2 | Hospedagem da plataforma IoT |
| Broker MQTT | Mosquitto | Distribuição de mensagens |
| Plataforma IoT | FIWARE | Integração e gerenciamento de dados |
| Context Broker | Orion | Gerenciamento das entidades |
| IoT Agent | UltraLight 2.0 | Conversão entre MQTT e NGSI-v2 |
| Histórico | STH-Comet | Consulta de dados históricos |
| Banco de dados | MongoDB | Persistência dos dados |
| Backend | ASP.NET Core 8 MVC | Regras de negócio e integração |
| Frontend | HTML, CSS e JavaScript | Interface com o usuário |

### Fluxo de aquisição de dados

1. Os sensores realizam as medições ambientais.
2. O ESP32 processa os dados coletados.
3. Os dados são publicados no Broker MQTT.
4. O IoT Agent interpreta as mensagens.
5. O Orion Context Broker atualiza a entidade.
6. O STH-Comet registra o histórico no MongoDB.
7. A aplicação web consulta as APIs do FIWARE.
8. Os dados são apresentados ao usuário no dashboard.

### Fluxo de acionamento dos alertas

1. A aplicação web consulta os dados ambientais.
2. O sistema calcula as médias das medições.
3. Os valores são comparados com os limites configurados.
4. Uma requisição HTTP PATCH é enviada ao Orion.
5. O IoT Agent encaminha o comando ao Broker MQTT.
6. O ESP32 recebe o comando.
7. O dispositivo aciona o LED e o buzzer.
8. O ESP32 publica a confirmação de execução.

---

## 3. Componentes utilizados

| Componente | Quantidade | Finalidade |
|---|---:|---|
| ESP32 | 1 | Processamento e comunicação Wi-Fi |
| DHT11 | 1 | Medição de temperatura e umidade |
| LDR | 1 | Medição relativa de luminosidade |
| LED azul | 1 | Sinalização visual |
| Buzzer passivo | 1 | Sinalização sonora |
| Resistores | Conforme montagem | Proteção e condicionamento |
| Jumpers | Conforme montagem | Conexões elétricas |
| Protoboard | 1 | Montagem do circuito |
| Cabo USB | 1 | Alimentação e programação |

---

## 4. Pinagem do hardware

A tabela abaixo representa a pinagem proposta para
o firmware de referência utilizando ESP32 DevKit V1.

| Componente | Pino | Tipo |
|---|---|---|
| DHT11 | GPIO 4 | Digital |
| LDR | GPIO 34 | Analógico |
| LED azul | GPIO 2 | Saída digital |
| Buzzer passivo | GPIO 25 | Saída PWM |
| Alimentação | USB | Alimentação |
| GND | GND | Terra comum |

A pinagem deverá ser validada de acordo com a montagem
física e o modelo do ESP32 utilizado.

A luminosidade é representada em uma escala percentual
relativa, não correspondendo diretamente a valores em lux.

---

## 5. Limites das variáveis ambientais

Os limites são configurados no arquivo appsettings.json
da aplicação ASP.NET Core.

| Variável | Valor mínimo | Valor máximo | Unidade |
|---|---:|---:|---|
| Temperatura | 15 | 30 | °C |
| Umidade relativa | 30 | 70 | % |
| Luminosidade | 20 | 80 | % |

Os valores mínimo e máximo são considerados inclusivos.

O sistema gera alertas quando as médias das medições
estão abaixo do limite mínimo ou acima do limite máximo.

O cálculo considera até 10 leituras numéricas recentes
de cada variável ambiental.

---

## 6. Tabela de alertas

| Código | Condição | Piscar do LED | Tom do buzzer |
|---:|---|---|---|
| 0 | Nenhuma média fora dos limites | Desligado | Silencioso |
| 1 | Temperatura fora dos limites | Rápido: 200 ms | 2000 Hz |
| 2 | Umidade fora dos limites | Médio: 400 ms | 1500 Hz |
| 3 | Luminosidade fora dos limites | Lento: 700 ms | 1000 Hz |

Os tempos de piscar representam o intervalo ligado
e o intervalo desligado do LED.

Os padrões apresentados correspondem ao firmware
de referência e devem ser validados no hardware físico.

### Prioridade dos alertas

Caso mais de uma variável esteja fora dos limites,
o sistema respeita a seguinte ordem de prioridade:

1. Temperatura;
2. Umidade;
3. Luminosidade.

O código 0 representa ausência de alertas nas medições
disponíveis, não necessariamente a confirmação de que
todos os sensores estão funcionando corretamente.

A aplicação depende da atualização do dashboard e da
opção de envio automático habilitada para encaminhar
os alertas automaticamente.

---

## 7. Comunicação MQTT

O protocolo MQTT é utilizado para comunicação entre
o dispositivo físico e a infraestrutura FIWARE.

### Configurações de referência

| Configuração | Valor |
|---|---|
| Protocolo | MQTT |
| Broker | Mosquitto |
| Porta | 1883 |
| API Key | TEF |
| Device ID | datalogger001 |
| FIWARE Service | smart |
| FIWARE ServicePath | / |
| Entity Type | DataLogger |
| Entity ID | urn:ngsi-ld:DataLogger:001 |

A API Key, o identificador do dispositivo e os tópicos
devem corresponder ao provisionamento do IoT Agent.

### Tópicos MQTT

| Tópico | Finalidade |
|---|---|
| /TEF/datalogger001/attrs | Publicação das medições |
| /TEF/datalogger001/cmd | Recebimento de comandos |
| /TEF/datalogger001/cmdexe | Confirmação de execução |

### Exemplo de publicação de telemetria

    t|25.4|h|61|l|72

Onde:

- t: temperatura;
- h: umidade relativa;
- l: luminosidade.

### Exemplo de comando recebido

    datalogger001@alerta|1

### Exemplo de confirmação de execução

    datalogger001@alerta|ok

---

## 8. Comunicação via API REST

A aplicação web realiza requisições HTTP para os
serviços disponibilizados pela plataforma FIWARE.

| Método HTTP | Finalidade |
|---|---|
| GET | Consultar entidades e medições |
| POST | Cadastrar dispositivos e assinaturas |
| PATCH | Atualizar atributos e enviar comandos |
| DELETE | Excluir recursos cadastrados |

### Exemplo de envio de alerta

Método: PATCH

Endpoint:

    /v2/entities/urn:ngsi-ld:DataLogger:001/attrs?type=DataLogger

Headers:

    Content-Type: application/json
    fiware-service: smart
    fiware-servicepath: /

Body:

    {
        "alerta": {
            "type": "command",
            "value": "1"
        }
    }

O retorno HTTP 204 indica que o comando foi aceito
pelo Orion Context Broker.

A execução física deve ser confirmada por meio do
tópico MQTT cmdexe e dos atributos de status do comando.

---

## 9. Deploy no AWS Learner Lab

### 9.1. Inicialização do laboratório

1. Acesse a plataforma AWS Academy.
2. Entre no AWS Learner Lab.
3. Clique em Start Lab.
4. Aguarde a inicialização do ambiente.
5. Acesse o AWS Management Console.
6. Pesquise pelo serviço EC2.

### 9.2. Configuração da instância EC2

Crie ou reutilize uma instância com as seguintes
configurações recomendadas:

| Configuração | Valor recomendado |
|---|---|
| Nome | fiware |
| Sistema operacional | Ubuntu Server LTS |
| Arquitetura | x86_64 |
| Tipo de instância | t3.small |
| Memória RAM | 2 GiB |
| Armazenamento | 20 GB |

A configuração depende dos recursos e das permissões
disponibilizadas pelo AWS Learner Lab.

Instâncias com arquitetura ARM/Graviton não são
suportadas pelas imagens do FIWARE Made Simple.

Uma instância t3.micro poderá ser utilizada para testes
com configuração de memória swap.

### 9.3. Conexão com a instância

1. Acesse EC2.
2. Selecione a instância criada.
3. Clique em Connect.
4. Selecione EC2 Instance Connect, quando disponível.
5. Abra o terminal da máquina virtual.

Antes de instalar, verifique se já existe uma
infraestrutura FIWARE em execução.

    uname -m
    free -h
    df -h
    docker ps -a

### 9.4. Instalação do Git

Caso necessário, execute:

    sudo apt update
    sudo apt install git -y

### 9.5. Download do FIWARE Made Simple

    git clone https://github.com/fabiocabrini/fiware-made-simple.git

Acesse a pasta:

    cd fiware-made-simple

### 9.6. Verificação dos requisitos

    ./scripts/check-requirements.sh --check-only

Execute o script de preparação:

    ./scripts/check-requirements.sh

O script realiza verificações e pode instalar ou
configurar os seguintes recursos:

- Docker Engine;
- Docker Compose;
- Dependências;
- Arquivo .env;
- Permissões do usuário;
- Memória swap.

Caso seja solicitado o encerramento da sessão:

    exit

Conecte-se novamente à instância e execute:

    cd ~/fiware-made-simple

### 9.7. Inicialização da plataforma FIWARE

    docker compose up -d --wait

Verifique os containers:

    docker compose ps

### 9.8. Validação da instalação

    ./scripts/smoke-test.sh

O teste verifica a comunicação entre os componentes
da plataforma FIWARE.

Também é possível consultar os serviços individualmente:

    curl http://localhost:1026/version
    curl http://localhost:4041/iot/about
    curl http://localhost:8666/version

Em caso de erro:

    docker compose logs --tail=100

### 9.9. Configuração do Security Group

| Porta | Protocolo | Serviço |
|---:|---|---|
| 22 | TCP | SSH |
| 1026 | TCP | Orion Context Broker |
| 1883 | TCP | Mosquitto MQTT |
| 4041 | TCP | IoT Agent |
| 7896 | TCP | IoT Agent HTTP |
| 8666 | TCP | STH-Comet |
| 9001 | TCP | MQTT WebSocket |

Autorize somente as portas necessárias e restrinja
o acesso aos endereços IP confiáveis.

Não disponibilize todas as portas para a internet
utilizando 0.0.0.0/0.

A plataforma possui finalidade acadêmica e não oferece,
por padrão, autenticação e criptografia suficientes
para exposição pública.

O MongoDB não deve ser exposto diretamente à internet.

### 9.10. Endereço da instância

Após a inicialização, obtenha o endereço IPv4 público
da instância EC2.

Esse endereço será utilizado na configuração do firmware
e na comunicação da aplicação web com o FIWARE.

O IP poderá mudar após a reinicialização do ambiente.

---

## 10. Como executar a aplicação web

### Pré-requisitos

- .NET SDK 8;
- Visual Studio 2022;
- Infraestrutura FIWARE em execução;
- Conexão com a internet;
- AWS Learner Lab, quando utilizada a descoberta da EC2.

### 10.1. Configuração do projeto

Abra o arquivo:

    DataLoggerWeb.csproj

Configure o arquivo:

    appsettings.json

Os principais parâmetros são:

- Região da AWS;
- Nome ou ID da instância EC2;
- Endereço do FIWARE;
- Portas dos serviços;
- FIWARE Service;
- FIWARE ServicePath;
- Limites das variáveis ambientais.

Para conexão direta por IP, configure:

    Aws:Habilitado = false

E informe o endereço do servidor em:

    Fiware:Host

### 10.2. Credenciais AWS

Caso seja utilizada a integração com o AWS SDK,
obtenha as credenciais temporárias no Learner Lab.

Acesse:

    AWS Details > AWS CLI > Show

Crie localmente o arquivo:

    .aws/credentials

As credenciais devem ser atualizadas conforme
a validade da sessão do laboratório.

Nunca publique esse arquivo no GitHub.

### 10.3. Executando pelo terminal

    dotnet restore
    dotnet run --launch-profile http

Acesse:

    http://localhost:5081

Também é possível executar o projeto diretamente
pelo Visual Studio utilizando a tecla F5.

### 10.4. Cadastro do Service Group

Na aplicação web, acesse Service Groups e configure:

| Campo | Valor de referência |
|---|---|
| API Key | TEF |
| Entity Type | DataLogger |
| Context Broker | http://orion:1026 |

O autoprovisionamento permanece desabilitado.

### 10.5. Cadastro do dispositivo

Na área de dispositivos, informe:

| Campo | Valor |
|---|---|
| Device ID | datalogger001 |
| Entity Name | urn:ngsi-ld:DataLogger:001 |
| Entity Type | DataLogger |
| Protocolo | UltraLight 2.0 |
| Transporte | MQTT |
| API Key | TEF |

Configure os atributos:

| Object ID | Atributo | Tipo |
|---|---|---|
| t | temperatura | Float |
| h | umidade | Float |
| l | luminosidade | Integer |

Configure também o comando:

    alerta

Crie as assinaturas responsáveis pelo armazenamento
do histórico de temperatura, umidade e luminosidade.

Evite duplicar assinaturas já existentes.

---

## 11. Como executar o dispositivo físico

### Pré-requisitos

- Arduino IDE;
- Pacote de placas ESP32;
- Biblioteca PubSubClient;
- Biblioteca DHT Sensor Library;
- Biblioteca Adafruit Unified Sensor.

### Configuração

Abra o firmware localizado em:

    firmware/DataLoggerESP32/DataLoggerESP32.ino

Copie o arquivo:

    config.example.h

Para:

    config.local.h

Configure:

- SSID da rede Wi-Fi;
- Senha da rede Wi-Fi;
- Endereço do Broker MQTT;
- Porta MQTT;
- API Key;
- Device ID.

Selecione o modelo correto da placa e a porta USB
correspondente no Arduino IDE.

Confira a pinagem, compile e carregue o firmware.

Abra o monitor serial com velocidade de:

    115200 baud

Verifique:

- Conexão Wi-Fi;
- Conexão MQTT;
- Leitura dos sensores;
- Publicação das medições;
- Recebimento dos comandos;
- Acionamento do LED;
- Acionamento do buzzer.

---

## 12. Interface web

A interface foi desenvolvida para permitir a supervisão
dos dispositivos IoT e a visualização das condições
ambientais monitoradas.

### Funcionalidades

- Dashboard de monitoramento;
- Gráficos de temperatura;
- Gráficos de umidade;
- Gráficos de luminosidade;
- Consulta de históricos;
- Cadastro de dispositivos;
- Gerenciamento de Service Groups;
- Gerenciamento de assinaturas;
- Identificação de alertas;
- Envio de comandos;
- Integração com a AWS;
- Atualização periódica das informações.

A aplicação utiliza JavaScript, AJAX e Chart.js
para atualização e visualização dos dados.

---

## 13. Estrutura do repositório

    DataLoggerIoT/
    |
    |-- Controllers/
    |-- DAO/
    |-- ViewModels/
    |-- Views/
    |-- wwwroot/
    |   |-- css/
    |   |   |-- site.css
    |   |
    |   |-- js/
    |       |-- dashboard.js
    |       |-- consulta.js
    |       |-- dispositivo.js
    |       |-- site.js
    |
    |-- firmware/
    |   |-- DataLoggerESP32/
    |       |-- DataLoggerESP32.ino
    |       |-- config.example.h
    |
    |-- docs/
    |   |-- arquitetura.svg
    |   |-- arquitetura.png
    |   |-- VALIDACAO.md
    |
    |-- Program.cs
    |-- appsettings.json
    |-- DataLoggerWeb.csproj
    |-- DOCUMENTACAO.md
    |-- MANUAL_CONFIGURACAO.md
    |-- MANUAL_USO.md
    |-- README.md
    |-- .gitignore

---

## 14. Documentação dos códigos

| Arquivo ou diretório | Responsabilidade |
|---|---|
| Program.cs | Inicialização da aplicação |
| Controllers/ | Rotas e operações do sistema |
| DAO/HelperFiware.cs | Comunicação HTTP com o FIWARE |
| DAO/EntidadeDAO.cs | Consulta e atualização de entidades |
| DAO/AlertaDAO.cs | Processamento dos alertas |
| DAO/AwsDAO.cs | Integração com AWS EC2 |
| Views/ | Interface web com Razor |
| dashboard.js | Atualização do dashboard e gráficos |
| consulta.js | Consulta de dados |
| dispositivo.js | Operações dos dispositivos |
| site.css | Estilização da aplicação |
| DataLoggerESP32.ino | Firmware de referência do dispositivo |

### Documentação complementar

- [Documentação técnica](DOCUMENTACAO.md)
- [Manual de configuração](MANUAL_CONFIGURACAO.md)
- [Manual de utilização](MANUAL_USO.md)
- [Roteiro de validação](docs/VALIDACAO.md)

---

## 15. Tecnologias utilizadas

- C++;
- C#;
- HTML;
- CSS;
- JavaScript;
- ASP.NET Core MVC;
- Razor;
- AJAX;
- Chart.js;
- Arduino IDE;
- ESP32;
- MQTT;
- UltraLight 2.0;
- API REST;
- FIWARE;
- Orion Context Broker;
- IoT Agent;
- STH-Comet;
- Mosquitto;
- MongoDB;
- Docker;
- Docker Compose;
- AWS EC2;
- Git e GitHub.

---

## 16. Segurança e boas práticas

Para garantir a segurança das informações e da
infraestrutura, não devem ser publicados:

- Credenciais da AWS;
- Senhas de redes Wi-Fi;
- Tokens de autenticação;
- Chaves privadas;
- Arquivos .env;
- Diretório .aws;
- Arquivo config.local.h.

Esses arquivos devem permanecer exclusivamente
no ambiente local e estar configurados no .gitignore.

O acesso aos serviços da AWS deve ser limitado
às origens autorizadas.

---

## 17. Possíveis melhorias futuras

- Implementação de autenticação de usuários;
- Notificações por e-mail;
- Aplicativo mobile;
- Alertas em tempo real;
- Configuração dinâmica dos limites;
- Exportação de relatórios;
- Comunicação MQTT com TLS;
- Monitoramento de múltiplos ambientes;
- Implementação de armazenamento local;
- Integração com ferramentas de Business Intelligence;
- Implementação de alertas locais independentes da rede.

---

## 18. Integrantes

| Integrante |
|---|
| Thiago Leite |
| João Paulo |
| Andreo Sampaio |
| Italo Silva |

**Curso:** Engenharia de Computação

**Disciplina:** Sistemas Embarcados

**Semestre:** 5º Semestre

---

## 19. Referências

- [FIWARE Foundation](https://www.fiware.org/)
- [FIWARE Made Simple — Fábio Henrique Cabrini](https://github.com/fabiocabrini/fiware-made-simple)
- [Arduino](https://www.arduino.cc/)
- [Amazon Web Services](https://aws.amazon.com/)
- [Docker](https://www.docker.com/)
- [MQTT](https://mqtt.org/)
- [Microsoft .NET](https://dotnet.microsoft.com/)
- [MongoDB](https://www.mongodb.com/)
- [Eclipse Mosquitto](https://mosquitto.org/)

---

## 20. Conclusão

O desenvolvimento do Data Logger IoT possibilita a
aplicação prática dos conhecimentos adquiridos na
disciplina de Sistemas Embarcados.

O projeto integra aquisição de dados, microcontroladores,
protocolos de comunicação, computação em nuvem,
bancos de dados e desenvolvimento de aplicações web.

A solução demonstra a aplicação dos conceitos de
Internet das Coisas no monitoramento ambiental,
permitindo a supervisão remota e o acionamento
de alertas no dispositivo físico.

---

## 21. Status do projeto

A aplicação web e a documentação estão organizadas
para publicação no GitHub.

O firmware disponibilizado é uma implementação
de referência.

A pinagem, os padrões de alerta, a compilação do
firmware e a integração completa com o hardware
e a AWS deverão ser validados antes da entrega final.

---

**Projeto acadêmico — Engenharia de Computação | 2026**
