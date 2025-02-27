# Sistema de Irrigação com Monitoramento Automatizado

Este projeto consiste em um sistema de irrigação automatizado voltado para a agricultura familiar. Ele utiliza um joystick para simular sensores de umidade e pH, exibe dados em tempo real em um display SSD1306 e controla a irrigação com base nas condições do solo. O sistema também inclui alertas visuais e sonoros para condições críticas.

## Funcionalidades

- **Simulação de Sensores**:
  - Eixo X do joystick: Simula um sensor de umidade (0-100%).
  - Eixo Y do joystick: Simula um sensor de pH (0-14).

- **Exibição de Dados**:
  - Display SSD1306: Mostra umidade, pH, modo de operação e status da irrigação.
  - Matriz de LEDs 5x5: Exibe símbolos correspondentes ao modo de operação (hortaliças, cactos, orquídeas).

- **Controle de Irrigação**:
  - Ativação automática da irrigação quando a umidade está abaixo do limite mínimo.
  - Desativação manual da irrigação pelo botão B.

- **Alertas**:
  - LEDs RGB: Indicam o nível de umidade com cores e brilho variados.
  - Buzzers: Emitem alertas sonoros para condições críticas (umidade baixa ou pH fora da faixa ideal).

## Hardware Utilizado

- **Microcontrolador**: Raspberry Pi Pico.
- **Sensores Simulados**: Joystick (eixos X e Y).
- **Display**: SSD1306 128x64px (comunicação I2C).
- **LEDs**: Matriz 5x5 de LEDs endereçáveis e LEDs RGB.
- **Buzzers**: Dois buzzers para alertas sonoros.
- **Botões**: Dois botões para controle manual (alternar modos e desligar irrigação).

## Software

- **Linguagem**: C/C++.
- **Bibliotecas**:
  - SSD1306 para controle do display.
  - font para exibição de texto no display
  - PWM para controle de LEDs e buzzers.
  - ADC para leitura dos eixos do joystick.
- **IDE**: Visual Studio Code com extensão Raspberry Pi Pico.

## Como Usar

### Compilação e Upload do Código

1. Instale o [Raspberry Pi Pico SDK](https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf).
2. Abra o projeto no Visual Studio Code.
3. Compile o código usando a extensão Raspberry Pi Pico.
4. Conecte o Raspberry Pi Pico ao computador e faça o upload do código.

### Operação

1. Ligue o sistema.
2. Use o joystick para simular os valores de umidade e pH.
3. O display mostrará os dados em tempo real.
4. Os LEDs RGB e a matriz de LEDs indicarão o modo de operação e o nível de umidade.
5. O sistema ativará automaticamente a irrigação se a umidade estiver abaixo do limite mínimo.
6. Use os botões para alternar entre os modos de operação e desligar manualmente a irrigação.

## Testes e Validação

O sistema foi testado em diferentes cenários para garantir o funcionamento correto:

1. **Leitura dos Sensores**: Verificação da leitura dos eixos X e Y do joystick.
2. **Exibição de Dados**: Teste da exibição correta dos dados no display SSD1306.
3. **Controle de LEDs**: Verificação das cores e brilho dos LEDs RGB.
4. **Controle de Irrigação**: Teste da ativação e desativação da irrigação.
5. **Alertas Sonoros**: Verificação dos alertas emitidos pelos buzzers.

## Resultados

O sistema mostrou-se estável e confiável durante os testes, atendendo aos requisitos propostos. Ele é adequado para uso em cenários reais de agricultura familiar, proporcionando eficiência e sustentabilidade no uso da água.

## Referências

- [Raspberry Pi Pico C/C++ SDK](https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf)
- [Adafruit SSD1306 Library Documentation](https://learn.adafruit.com/adafruit-oled-featherwing)
- [Analog pH Sensor for Soil](https://wiki.dfrobot.com/Analog_pH_Sensor_Kit_SKU_SEN0161_V2)
- [Precision agriculture—a worldwide overview](https://doi.org/10.1016/S0168-1699(02)00096-0)
- [The State of Food and Agriculture 2020](https://www.fao.org/3/cb1447en/cb1447en.pdf)

## Autor

- **Eder Renato da Silva Cardoso Casar**
- **Data**: 25 de Fevereiro de 2025

## Video Demonstrativo

https://youtu.be/5bY9tzJwBFE