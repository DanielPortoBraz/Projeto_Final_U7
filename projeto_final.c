// ------Bibliotecas------
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/i2c.h"

#include "ws2818b.pio.h" 
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "animacoes.h"
#include "images.h"



// -----Pinos e valores padõres------

// Níveis extremos de umidade e saturação do oxigênio do ser humano
#define HR_MAX 60
#define HR_MIN 40
#define SPO2_MIN 90


// Buzzer
const uint8_t BUZZER_PIN = 21;
const uint16_t PERIOD = 17750; // WRAP
const float DIVCLK = 16.0; // Divisor inteiro
static uint slice_21;
const uint16_t dc_values[] = {PERIOD * 0.3, 0}; // Duty Cycle de 30% e 0%
static volatile uint16_t dc_level = PERIOD * 0.3; // Nível do Duty Cycle
static volatile bool toogle = false;

// Joystick
const uint8_t VRx = 27;
const uint8_t VRy = 26;
const uint8_t ADC_CHAN_0 = 0;
const uint8_t ADC_CHAN_1 = 1; 
static uint16_t vrx_value;
static uint16_t vry_value;
static uint16_t half_adc = 2048;
static uint16_t oxygen;
static uint16_t humidity;

// Matriz de LEDs 5x5
#define LED_COUNT 25
#define LED_PIN 7
struct pixel_t { // Estrutura de cada LED GRB (Ordem do ws2818b)
    uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; 
npLED_t leds[LED_COUNT]; // Buffer de pixels da matriz
PIO np_pio; 
uint sm;

// Display OLED 1306
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
ssd1306_t ssd; // Inicializa a estrutura do display


// Botões
#define BUTTON_A_PIN 5 // Alterna o PWM do buzzer
#define BUTTON_B_PIN 6 // Inicia o modo de coleta de dados
static volatile uint32_t last_time = 0; 

// LED RGB
const uint8_t leds_pins[] = {13, 11, 12};



// ----------- FUNÇÕES -------------

// ------ Funções do PWM ------

void setup_pwm(){

    // PWM do BUZZER
    // Configura para soar 440 Hz
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    slice_21 = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_clkdiv(slice_21, DIVCLK);
    pwm_set_wrap(slice_21, PERIOD);
    pwm_set_gpio_level(BUZZER_PIN, 0);
    pwm_set_enabled(slice_21, true);
}


// ------ Funções do Joystick -------

// Inicializa o joystick e os conversores ADC
void init_joystick(){
    adc_init();
    adc_gpio_init(VRx);
    adc_gpio_init(VRy);
}

// Obtém o valor atual das coordenadas do joystick com base na leitura ADC 
void read_joystick(uint16_t *vrx_value, uint16_t *vry_value){
    adc_select_input(ADC_CHAN_1);
    sleep_us(2);
    *vrx_value = adc_read();

    adc_select_input(ADC_CHAN_0);
    sleep_us(2);
    *vry_value = adc_read();
}


// ------Funções da matriz de LEDs ws2818b------

/**
 * Inicializa a máquina PIO para controle da matriz de LEDs.
 */
void npInit(uint pin) {

    // Cria programa PIO.
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    // Seleciona uma das máquinas de estado livres
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true); 
    }

    // Inicia programa na máquina PIO obtida.
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    // Limpa buffer de pixels.
    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}


// Função para converter a posição da matriz para uma posição do vetor.
int getIndex(int x, int y) {
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
    if (y % 2 == 0) {
        return 24-(y * 5 + x); // Linha par (esquerda para direita).
    } else {
        return 24-(y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
    }
}


/**
 * Atribui uma cor RGB a um LED.
 */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

/**
 * Limpa o buffer de pixels.
 */
void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
}

/**
 * Escreve os dados do buffer nos LEDs.
 */
void npWrite() {
    // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}
  

// ------Funções do display OLED ssd1306------

void initialize_i2c(){
    i2c_init(I2C_PORT, 400 * 1000); // Frequência de transmissão de 400 khz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // GPIO para função de I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // GPIO para função de I2C
    gpio_pull_up(I2C_SDA); 
    gpio_pull_up(I2C_SCL); 
}

// Apaga uma mensagem no display
void erase_msg(char *c, uint8_t x, uint8_t y){
    int tam = strlen(c); 
    char eraser[tam + 1];

    for(int i = 0; i < tam; i++)
        eraser[i] = ' ';

    eraser[tam] = '\0';
     
    ssd1306_draw_string(&ssd, eraser, x, y); 
    ssd1306_send_data(&ssd);
}

// Desenha os valores das porcentagens
void draw_percs(){

    // Apaga os valores anteriores
    erase_msg("100", 24, 12);
    erase_msg("100", 63, 12);

    // Escreve os novos valores no display
    char per_oxygen[4], per_humidity[4];
    sprintf(per_oxygen, "%d", oxygen);
    sprintf(per_humidity, "%d", humidity);
    ssd1306_draw_string(&ssd, per_oxygen, 24, 12);
    ssd1306_draw_string(&ssd, per_humidity, 63, 12);
    ssd1306_send_data(&ssd);
}


// ------Funções dos botões------

void init_buttons(){
    gpio_init(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_pull_up(BUTTON_B_PIN);
}

// Função callback de interrupção
void gpio_irq_handler(uint gpio, uint32_t events){
    
    // Guarda o tempo em us desde o boot do sistema
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    if(current_time - last_time > 200) { // Efeito de debounce gerado pelo atraso de 200 ms na leitura do botão
        last_time = current_time;

        // Ativa/desativa o som do Buzzer
        toogle = !toogle;
        dc_level = dc_values[toogle];
    }
}   


// ------Funções do LED RGB------
void init_leds(){
    for (int i = 0; i < 3; i++){
        gpio_init(leds_pins[i]);
        gpio_set_dir(leds_pins[i], GPIO_OUT);
        gpio_put(leds_pins[i], false);
    }
}



// >>>>> ROTINAS <<<<<<

// Coleta das amostras
int collection(void){

    // Acende LED azul para indicar rotina
    gpio_put(leds_pins[2], true);

    // Gera animação de coleta na matriz de LEDs
    for(int k = 0; k <  10; k++){

        for (int v = 0; v < 3; v++){

            for(int tam = 0; tam < LED_COUNT; tam++){
                npSetLED(tam, breath[v][24 - tam][0], breath[v][24 - tam][1], breath[v][24 - tam][2]);
                npWrite();
            }
            sleep_ms(200);
        }

        npClear();
        npWrite();

        // Lê os valores dos sensores de oxigênio e umidade
        read_joystick(&vrx_value, &vry_value);

        // Garante um intervalo de sensibilidade, evita leituras precipitadas
        oxygen = (vrx_value * 100) / 4095; // Porcentagem de oxigênio
        humidity = (vry_value * 100) / 4095; // Porcentagem de umidade

        //Exibe as leituras no display
        ssd1306_levels(&ssd, oxygen, 11, 59); // Barra do Oxigênio
        ssd1306_levels(&ssd, humidity, 49, 59); // Barra da Umidade 
        draw_percs();
    }

    // Desliga o LED azul
    gpio_put(leds_pins[2], false);
} 


// Liberação de O2/HR
void liberation(){

    // Acende LED azul para indicar rotina
    gpio_put(leds_pins[2], true);

    // Libera o O2 e HR até normalizar os níveis
    while(oxygen <= SPO2_MIN || humidity <= HR_MIN || humidity >= HR_MAX){

        // Gera animação de liberação na matriz de LEDs (animação de coleta invertida)
        for (int v = 0; v < 3; v++){

            for(int tam = 0; tam < LED_COUNT; tam++){
                npSetLED(tam, breath[2 - v][24 - tam][0], breath[2 - v][24 - tam][1], breath[2 - v][24 - tam][2]);
                npWrite();
            }
            npWrite();
            sleep_ms(200);
        }
    
        npClear();
        npWrite();

        // Se o oxigênio estiver abaixo de 92%, aumenta o nível em 10%
        if(oxygen <= SPO2_MIN)
            oxygen += 10;

        // Se a umidade estiver abaixo de 40%, aumenta o nível em 10%
        if(humidity <= HR_MIN)
            humidity += 10;
        
        // Se a umidade estiver abaixo de 60%, diminui o nível em 10%
        if(humidity >= HR_MAX)
            humidity -= 10;

        ssd1306_levels(&ssd, oxygen, 11, 59); // Barra de Oxigênio
        ssd1306_levels(&ssd, humidity, 49, 59); // Barra de Umidade
        draw_percs();
        
        sleep_ms(500);
    }

    // Aviso sonoro de final da rotina
    pwm_set_gpio_level(BUZZER_PIN, dc_level);
    sleep_ms(800);
    pwm_set_gpio_level(BUZZER_PIN, 0);

    gpio_put(leds_pins[2], false);
}


// ======== Programa principal =========
int main()
{
    stdio_init_all();

    // Inicializações
    initialize_i2c();
    npInit(LED_PIN);
    setup_pwm();
    init_joystick();
    init_buttons();
    init_leds();

    // Configuração do Display OLED 1306
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Limpa a matriz de LEDs
    npClear();
    npWrite();

    // Interrupção do Botão A
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);


    while (true) { 

        // Exibe display inicial
        ssd1306_draw_bitmap(&ssd, img_default);
        ssd1306_draw_string(&ssd, "HELLO", 75, 38);
        ssd1306_draw_string(&ssd, "PRESS B", 75, 46);
        ssd1306_send_data(&ssd);

        // Se o botão B for apertado, as rotinas são iniciadas
        if(!gpio_get(BUTTON_B_PIN)){ 
            sleep_ms(50); // Tempo para debounce

            if(!gpio_get(BUTTON_B_PIN)){

                // Apaga a mensagem anterior
                erase_msg("HELLO", 75, 38);
                erase_msg("PRESS B", 75, 46);

                // Inicia o modo de coleta e vai atualizando o display
                collection();

                // Se os níveis estiverem anormais, inicia a rotina de liberação
                if (oxygen <= SPO2_MIN || humidity <= HR_MIN || humidity >= HR_MAX){

                    // Alerta sonoro e LED vermelho piscando para indicar anormalidade
                    for(int i = 0; i < 3; i++){
                        gpio_put(leds_pins[0], true);
                        pwm_set_gpio_level(BUZZER_PIN, dc_level);
                        sleep_ms(200);
                        pwm_set_gpio_level(BUZZER_PIN, 0);
                        gpio_put(leds_pins[0], false);
                        sleep_ms(200);
                    }

                    // Exibe a mensgem de qual nível está anormal 
                    if(oxygen <= SPO2_MIN)
                        ssd1306_draw_string(&ssd, "O2 LOW", 75, 38);

                    if (humidity <= HR_MIN)
                        ssd1306_draw_string(&ssd, "HR LOW", 75, 46);

                    if(humidity >= HR_MAX)
                        ssd1306_draw_string(&ssd, "HR HIGH", 75, 46);

                    ssd1306_send_data(&ssd);
                    sleep_ms(3000);

                    // Apaga a mensagem anterior
                    erase_msg("O2 LOW", 75, 38);
                    erase_msg("HR HIGH", 75, 46);

                    // Aciona a rotina de liberação
                    liberation();
                }

                else{

                    // Sinaliza situação de níveis normais
                    ssd1306_draw_string(&ssd, "LEVELS OK", 75, 46);
                    ssd1306_send_data(&ssd);
                    
                    gpio_put(leds_pins[1], true); // Acende o LED verde
                    sleep_ms(3000);
                    erase_msg("LEVELS OK", 75, 46);
                    gpio_put(leds_pins[1], false);
                }
            }
        }
        sleep_ms(200);
    }
}


