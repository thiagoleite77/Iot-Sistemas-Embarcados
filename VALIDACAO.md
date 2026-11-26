
# Roteiro de Validação — Data Logger IoT

## 1. Identificação do projeto

**Curso:** Engenharia de Computação  
**Disciplina:** Sistemas Embarcados  
**Semestre:** 5º Semestre

### Integrantes

- Thiago Leite
- João Paulo
- Andreo Sampaio
- Italo Silva

---

## 2. Objetivo

Validar o funcionamento dos componentes de hardware,
firmware, comunicação MQTT, infraestrutura FIWARE,
aplicação web e sistema de alertas.

Os testes têm como objetivo verificar a integração
entre o dispositivo físico e os serviços hospedados
no AWS Learner Lab.

Todos os testes devem possuir evidências de execução.

---

## 3. Validação do hardware

| ID | Teste | Resultado esperado | Status |
|---|---|---|---|
| HW01 | Alimentação do ESP32 | Inicialização correta | Pendente |
| HW02 | Pinagem dos componentes | Compatível com a placa | Pendente |
| HW03 | Sensor DHT11 | Temperatura válida | Pendente |
| HW04 | Sensor DHT11 | Umidade válida | Pendente |
| HW05 | Sensor LDR | Variação conforme iluminação | Pendente |
| HW06 | LED | Acionamento correto | Pendente |
| HW07 | Buzzer | Emissão sonora | Pendente |
| HW08 | Circuito completo | Funcionamento integrado | Pendente |

---

## 4. Validação do firmware

| ID | Teste | Resultado esperado | Status |
|---|---|---|---|
| FW01 | Compilação | Nenhum erro de compilação | Pendente |
| FW02 | Upload | Firmware carregado na placa | Pendente |
| FW03 | Monitor serial | Informações disponíveis | Pendente |
| FW04 | Conexão Wi-Fi | Dispositivo conectado | Pendente |
| FW05 | Conexão MQTT | Broker conectado | Pendente |
| FW06 | Publicação periódica | Medições a cada 5 segundos | Pendente |
| FW07 | Recebimento MQTT | Comandos recebidos | Pendente |
| FW08 | Confirmação MQTT | Publicação em cmdexe | Pendente |
| FW09 | Comando inválido | Comando ignorado | Pendente |
| FW10 | Reconexão | Comunicação restabelecida | Pendente |

---

## 5. Validação da infraestrutura AWS

Acessar o terminal da instância EC2.

Entrar no diretório do FIWARE:

    cd ~/fiware-made-simple

Verificar os containers:

    docker compose ps

Executar o teste automatizado:

    ./scripts/smoke-test.sh

Consultar o Orion:

    curl http://localhost:1026/version

Consultar o IoT Agent:

    curl http://localhost:4041/iot/about

Consultar o STH-Comet:

    curl http://localhost:8666/version

### Checklist

| ID | Teste | Resultado esperado | Status |
|---|---|---|---|
| AWS01 | AWS Learner Lab | Laboratório iniciado | Pendente |
| AWS02 | Instância EC2 | Instância em execução | Pendente |
| AWS03 | Docker | Serviço disponível | Pendente |
| AWS04 | Docker Compose | Containers saudáveis | Pendente |
| AWS05 | Orion | Resposta HTTP válida | Pendente |
| AWS06 | IoT Agent | Resposta HTTP válida | Pendente |
| AWS07 | Mosquitto | Broker disponível | Pendente |
| AWS08 | MongoDB | Banco operacional | Pendente |
| AWS09 | STH-Comet | Serviço operacional | Pendente |
| AWS10 | Smoke Test | Zero falhas | Pendente |
| AWS11 | Security Group | Portas restritas | Pendente |
| AWS12 | Nova sessão AWS | IP e credenciais atualizados | Pendente |

---

## 6. Validação do provisionamento

Verificar o cadastro do Service Group e do dispositivo.

### Configurações de referência

| Parâmetro | Valor |
|---|---|
| API Key | TEF |
| Device ID | datalogger001 |
| Entity Type | DataLogger |
| Entity ID | urn:ngsi-ld:DataLogger:001 |
| Transporte | MQTT |
| Protocolo | UltraLight 2.0 |

### Atributos cadastrados

| Object ID | Atributo |
|---|---|
| t | temperatura |
| h | umidade |
| l | luminosidade |

### Comando cadastrado

    alerta

| ID | Teste | Resultado esperado | Status |
|---|---|---|---|
| PR01 | Service Group | Cadastrado corretamente | Pendente |
| PR02 | Dispositivo | Cadastrado corretamente | Pendente |
| PR03 | Atributos | Temperatura, umidade e luminosidade | Pendente |
| PR04 | Comando alerta | Disponível no Orion | Pendente |
| PR05 | Assinaturas | Histórico das três variáveis | Pendente |

---

## 7. Validação da comunicação MQTT

### Tópico de telemetria

    /TEF/datalogger001/attrs

### Tópico de comandos

    /TEF/datalogger001/cmd

### Tópico de confirmação

    /TEF/datalogger001/cmdexe

### Exemplo de telemetria

    t|25.4|h|50.0|l|60

### Resultados esperados

| ID | Teste | Resultado esperado | Status |
|---|---|---|---|
| MQ01 | Publicação de temperatura | Orion atualizado | Pendente |
| MQ02 | Publicação de umidade | Orion atualizado | Pendente |
| MQ03 | Publicação de luminosidade | Orion atualizado | Pendente |
| MQ04 | Recebimento de comando | ESP32 recebe mensagem | Pendente |
| MQ05 | Confirmação | Mensagem em cmdexe | Pendente |
| MQ06 | Histórico | Leituras registradas | Pendente |

---

## 8. Validação dos alertas

### Limites ambientais

| Variável | Mínimo | Máximo |
|---|---:|---:|
| Temperatura | 15 °C | 30 °C |
| Umidade | 30% | 70% |
| Luminosidade | 20% | 80% |

Os limites são inclusivos.

Os padrões abaixo correspondem ao firmware de
referência e devem ser confirmados no hardware real.

| Código | Condição | LED | Buzzer |
|---|---|---|---|
| 0 | Nenhuma média fora da faixa | Desligado | Silencioso |
| 1 | Temperatura irregular | 200 ms | 2000 Hz |
| 2 | Umidade irregular | 400 ms | 1500 Hz |
| 3 | Luminosidade irregular | 700 ms | 1000 Hz |

### Cenários de teste

Os valores representam as médias das medições
consideradas pela aplicação.

| ID | Temperatura | Umidade | Luz | Alerta esperado | Status |
|---|---:|---:|---:|---|---|
| AL01 | 25 °C | 50% | 60% | Normal - código 0 | Pendente |
| AL02 | 35 °C | 50% | 60% | Temperatura - código 1 | Pendente |
| AL03 | 10 °C | 50% | 60% | Temperatura - código 1 | Pendente |
| AL04 | 25 °C | 80% | 60% | Umidade - código 2 | Pendente |
| AL05 | 25 °C | 20% | 60% | Umidade - código 2 | Pendente |
| AL06 | 25 °C | 50% | 90% | Luminosidade - código 3 | Pendente |
| AL07 | 25 °C | 50% | 10% | Luminosidade - código 3 | Pendente |
| AL08 | 35 °C | 80% | 90% | Temperatura - código 1 | Pendente |
| AL09 | 25 °C | 80% | 90% | Umidade - código 2 | Pendente |
| AL10 | 15 °C | 30% | 20% | Normal - código 0 | Pendente |
| AL11 | 30 °C | 70% | 80% | Normal - código 0 | Pendente |

### Prioridade dos alertas

1. Temperatura;
2. Umidade;
3. Luminosidade.

### Teste de retorno à normalidade

Após a ativação de um alerta, restabelecer os
valores das médias para dentro dos limites.

Resultado esperado:

- Código de alerta igual a 0;
- LED desligado;
- Buzzer silencioso;
- Confirmação enviada ao FIWARE.

---

## 9. Validação da API REST

Utilizar o Postman para verificar o envio de comandos.

### Método HTTP

    PATCH

### Endpoint de referência

    http://IP_DA_AWS:1026/v2/entities/urn:ngsi-ld:DataLogger:001/attrs?type=DataLogger

### Headers

    Content-Type: application/json
    fiware-service: smart
    fiware-servicepath: /

### Body — Alerta de temperatura

    {
        "alerta": {
            "type": "command",
            "value": "1"
        }
    }

### Body — Desativar alerta

    {
        "alerta": {
            "type": "command",
            "value": "0"
        }
    }

### Resultado esperado

- HTTP 204 No Content;
- Comando encaminhado pelo IoT Agent;
- Mensagem publicada no Broker MQTT;
- ESP32 recebe o comando;
- LED e buzzer assumem o padrão correspondente;
- ESP32 publica a confirmação em cmdexe.

O HTTP 204, isoladamente, não comprova a execução
física do comando.

---

## 10. Validação da aplicação web

Executar a compilação:

    dotnet build

Executar a aplicação:

    dotnet run --launch-profile http

### Checklist

| ID | Teste | Resultado esperado | Status |
|---|---|---|---|
| WEB01 | Compilação | Sem erros | Pendente |
| WEB02 | Inicialização | Aplicação disponível | Pendente |
| WEB03 | Dashboard | Informações exibidas | Pendente |
| WEB04 | Temperatura | Gráfico atualizado | Pendente |
| WEB05 | Umidade | Gráfico atualizado | Pendente |
| WEB06 | Luminosidade | Gráfico atualizado | Pendente |
| WEB07 | Histórico | Dados consultados | Pendente |
| WEB08 | Cálculo das médias | Resultados corretos | Pendente |
| WEB09 | Limites | Validação correta | Pendente |
| WEB10 | Envio de alertas | Comando enviado | Pendente |
| WEB11 | Responsividade | Interface adaptável | Pendente |
| WEB12 | Falha de conexão | Comportamento registrado | Pendente |

---

## 11. Validação de falhas e reconexão

| ID | Cenário | Verificação | Status |
|---|---|---|---|
| FL01 | Desconectar Wi-Fi | Reconexão automática | Pendente |
| FL02 | Interromper Broker MQTT | Nova tentativa de conexão | Pendente |
| FL03 | Restabelecer Broker | Telemetria e inscrição retomadas | Pendente |
| FL04 | Desconectar DHT11 | Leituras inválidas descartadas | Pendente |
| FL05 | Reiniciar ESP32 | Reinicialização do firmware | Pendente |
| FL06 | Comando MQTT inválido | Comando ignorado | Pendente |
| FL07 | Fechar dashboard | Registrar comportamento dos alertas | Pendente |
| FL08 | Reiniciar AWS | Atualizar IP quando necessário | Pendente |

A ausência de medições não deve ser interpretada
como confirmação de que o ambiente está normal.

O firmware de referência mantém o último alerta
recebido durante falhas de comunicação.

O acionamento automático de novos alertas depende
do funcionamento da aplicação web e da infraestrutura
de comunicação.

---

## 12. Validação ponta a ponta

Executar o fluxo completo:

1. Energizar o ESP32.
2. Verificar a conexão Wi-Fi.
3. Verificar a conexão MQTT.
4. Realizar medições ambientais.
5. Publicar os dados no Broker MQTT.
6. Verificar atualização no Orion.
7. Verificar histórico no STH-Comet.
8. Abrir a aplicação web.
9. Verificar os dados no dashboard.
10. Habilitar o envio automático de alertas.
11. Provocar uma condição fora dos limites.
12. Verificar o cálculo das médias.
13. Verificar o envio do comando.
14. Confirmar recebimento no ESP32.
15. Verificar acionamento do LED e buzzer.
16. Verificar a confirmação MQTT.
17. Restabelecer os valores normais.
18. Confirmar a desativação do alerta.

---

## 13. Evidências de execução

Registrar as evidências dos testes realizados.

| ID do teste | Data | Responsável | Resultado | Evidência |
|---|---|---|---|---|
| | | | | |
| | | | | |
| | | | | |

As evidências podem ser armazenadas em:

    docs/evidencias/

Exemplos:

    aws-containers.png
    mqtt-conexao.png
    dashboard.png
    postman-patch.png
    alerta-temperatura.mp4
    hardware-completo.jpg

Não incluir credenciais, senhas ou tokens
nas capturas de tela.

---

## 14. Checklist de entrega GitHub

- [ ] README.md completo
- [ ] Objetivo do projeto
- [ ] Componentes utilizados
- [ ] Pinagem confirmada
- [ ] Limites ambientais
- [ ] Tabela de alertas
- [ ] Diagrama da arquitetura
- [ ] Deploy AWS Learner Lab
- [ ] Instruções de execução
- [ ] Firmware comentado
- [ ] Frontend comentado
- [ ] Documentação técnica
- [ ] Roteiro de validação
- [ ] Evidências dos testes
- [ ] Nome dos integrantes
- [ ] Arquivo .gitignore
- [ ] Ausência de credenciais AWS
- [ ] Ausência de senhas Wi-Fi
- [ ] Repositório GitHub publicado
- [ ] Link enviado pelo Microsoft Forms

---

## 15. Aprovação final

| Responsável | Validação | Data |
|---|---|---|
| Thiago Leite | Pendente | |
| João Paulo | Pendente | |
| Andreo Sampaio | Pendente | |
| Italo Silva | Pendente | |

### Considerações finais

O projeto deverá ser considerado validado após
a execução dos testes de hardware, firmware,
comunicação, infraestrutura AWS e aplicação web.

Os resultados deverão ser documentados e acompanhados
das respectivas evidências.

**Engenharia de Computação — Sistemas Embarcados — 2026**
