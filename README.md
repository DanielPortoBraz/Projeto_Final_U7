# Dispositivo Regulador de Oxigênio e Umidade

## Descrição
Este projeto consiste em um sistema embarcado para monitoramento e regulação dos níveis de saturação de oxigênio (SpO2) e umidade relativa do ar de um indivíduo. O dispositivo atua automaticamente ao detectar valores fora do intervalo seguro, ativando mecanismos para normalizar os níveis de oxigenação e umidade.

## Objetivos
- Monitorar os níveis de oxigenação e umidade em tempo real.
- Regular os níveis conforme necessário.
- Emitir alertas visuais e sonoros em caso de anormalidades.
- Melhorar a qualidade de vida de pessoas com doenças respiratórias.

## Estrutura dos Arquivos
- **projeto_final.c**: Código principal do firmware, responsável pela leitura dos sensores, controle dos atuadores e exibição de informações no display.
- **ws2818b.pio.h**: Biblioteca para controle da matriz de LEDs WS2818B usando PIO (Programável I/O) do RP2040.
- **inc/ssd1306.h**: Biblioteca para comunicação com o display OLED SSD1306 via protocolo I2C.
- **inc/font.h**: Definições de fontes utilizadas no display OLED.
- **animacoes.h**: Definições de animações para a matriz de LEDs.
- **images.h**: Imagens pré-definidas utilizadas no display OLED.

## Hardware Utilizado
- **Microcontrolador**: ATmega328P
- **Sensores**: MAX30102 (oxigenação), SHTC3 (umidade)
- **Atuadores**: Válvula solenoide biestável, microbomba peristáltica DC, buzzer
- **Periféricos**: Display OLED SSD1306 (I2C), Matriz de LEDs WS2818B, LEDs RGB
- **Botões**: Dois botões para controle de funções
- **Fonte de alimentação**: Bateria Li-Ion 3.7V

## Funcionalidades
1. **Coleta de Dados**: O usuário inicia a medição ao pressionar um botão.
2. **Exibição das Leituras**: Os valores são mostrados no display OLED em tempo real.
3. **Verificação de Anormalidades**: O sistema analisa os dados e identifica desvios dos níveis normais.
4. **Alerta ao Usuário**: Em caso de valores críticos, LEDs vermelhos piscam e um sinal sonoro é ativado.
5. **Regulação Automática**: Se necessário, o sistema ativa a liberação de oxigênio e/ou umidade até os níveis se estabilizarem.
6. **Indicação de Normalidade**: Um LED verde acende caso os valores estejam dentro da faixa ideal.

## Estrutura do Código
O código principal está estruturado em diferentes módulos:
- **Configuração dos sensores e periféricos**
- **Leitura e processamento dos dados dos sensores**
- **Lógica de decisão e controle dos atuadores**
- **Interface com o usuário via display OLED e LEDs**

## Como Compilar e Executar
1. Instale as dependências para compilação do código C.
2. Utilize o ambiente de desenvolvimento apropriado para microcontroladores (VS Code + Extensões Pico/RP2040, Arduino IDE, etc.).
3. Compile o código e carregue no microcontrolador.
4. Conecte os sensores e atuadores conforme a especificação do hardware.
5. Ligue o dispositivo e pressione o botão de coleta para iniciar a medição.


## Vídeo de demonstração: https://youtu.be/1dCOAVGKm1U?si=z-8TQcQN6Vf3Ad1i

# Licença - Todos os Direitos Reservados

© 2024 [Daniel Porto Braz] - Todos os direitos reservados.

Este projeto e seus arquivos associados são de propriedade exclusiva do autor. Nenhuma parte deste projeto pode ser copiada, distribuída, modificada ou utilizada para fins comerciais ou não comerciais sem autorização expressa por escrito do autor.

A reprodução, redistribuição ou uso não autorizado deste projeto, total ou parcial, está estritamente proibida e sujeita às penalidades previstas em lei.

Para obter permissão de uso, entre em contato com o autor através de **[danielpb132@gmail.com]**.

📍 **[Feira de Santana, 25/02/2025]**  
✍️ **[Daniel Porto Braz]**
