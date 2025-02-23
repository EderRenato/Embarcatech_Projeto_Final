#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2818b.pio.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/watchdog.h"

#define LED_COUNT 25                // Numero de leds da matriz
#define MATRIX_PIN 7                // Pino da matriz de LEDs
#define I2C_PORT i2c1               // Porta i2c1
#define DISPLAY_SDA 14              // Pino do SDA
#define DISPLAY_SCL 15              // Pino do SCL
#define ENDERECO 0x3C
const uint GREEN_LED = 11;
const uint BLUE_LED = 12;
const uint RED_LED = 13;
const uint X_AXIS = 26;             //eixo x do joystick
const uint Y_AXIS = 27;             //eixo y do joystick
const uint BUZZER_A = 21;
const uint BUZZER_B = 10;
const uint BUTTON_A = 5;
const uint BUTTON_B = 6;
const uint DEBOUNCE_DELAY = 150;   // Tempo de debounce para os botões
const uint FREQUENCY = 50;   // Frequência do PWM
const uint WRAP = (1000000 / FREQUENCY);  // Período em microssegundos

uint NIVEL_VIBRACAO = 0;
uint INCLINACAO = 0;
uint CARGA = 0;
uint NIVEL_ALERTA = 0;
bool CALIBRADO = false;

const int LIMITE_VIBRACAO_LEVE = 30;
const int LIMITE_VIBRACAO_MODERADO = 60;
const int LIMITE_INCLINACAO = 70;
const int LIMITE_CARGA = 80;

ssd1306_t ssd; // Inicialização a estrutura do display

struct pixel_t {
    uint8_t G, R, B;        // Componentes de cor: Verde, Vermelho e Azul
};

typedef struct pixel_t pixel_t; // Alias para a estrutura pixel_t
typedef pixel_t npLED_t;        // Alias para facilitar o uso no contexto de LEDs

npLED_t leds[LED_COUNT];        // Array para armazenar o estado de cada LED
PIO np_pio;                     // Variável para referenciar a instância PIO usada
uint sm;                        // Variável para armazenar o número do state machine usado
// Função para calcular o índice do LED na matriz
int getIndex(int x, int y);
// Função para inicializar o PIO para controle dos LEDs
void npInit(uint pin);
// Função para definir a cor de um LED específico
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b);
// Função para limpar (apagar) todos os LEDs
void npClear();
// Função para atualizar os LEDs no hardware
void npWrite();

void init_buzzer_pwm(uint gpio); // funcção para inicializar os buzzers como pwm
void init_led_pwm(uint gpio); // função para inicializar os leds como pwm

void set_buzzer_tone(uint gpio, uint freq);
void stop_buzzer(uint gpio);
void set_led_pulse(uint gpio, uint16_t percentage); // envio de dados do pwm
void reset();
void buttons_callback(uint gpio, uint32_t events){
    static uint32_t button_a_last_time = 0;
    static uint32_t button_b_last_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    if (gpio == BUTTON_A) {
        if (current_time - button_a_last_time > DEBOUNCE_DELAY) {
            button_a_last_time = current_time;
            stop_buzzer(BUZZER_A);
            stop_buzzer(BUZZER_B);
        }
    } else if (gpio == BUTTON_B) {
        if (current_time - button_b_last_time > DEBOUNCE_DELAY) {
            button_b_last_time = current_time;
            reset();
        }
    }
}
void display_init(); //função para inicializar o display
void init_hardware(); //função para inicializar todos os componentes

int main()
{
    stdio_init_all();
    init_hardware();
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, buttons_callback);
    gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);
    while (true) {
        set_led_pulse(GREEN_LED, 10);
        sleep_ms(100);
        set_led_pulse(GREEN_LED, 20);
        sleep_ms(100);
        set_led_pulse(GREEN_LED, 30);
        sleep_ms(100);
        set_led_pulse(GREEN_LED, 40);
        sleep_ms(100);
        set_led_pulse(GREEN_LED, 50);
        sleep_ms(100);
        set_led_pulse(GREEN_LED, 60);
        sleep_ms(100);
        set_led_pulse(GREEN_LED, 70);
        sleep_ms(100);
        set_led_pulse(GREEN_LED, 80);
        sleep_ms(100);
        set_led_pulse(GREEN_LED, 90);
        sleep_ms(100);
        set_led_pulse(GREEN_LED, 100);
        sleep_ms(100);
    }
}

int getIndex(int x, int y) {
    x = 4 - x; // Inverte as colunas (0 -> 4, 1 -> 3, etc.)
    y = 4 - y; // Inverte as linhas (0 -> 4, 1 -> 3, etc.)
    if (y % 2 == 0) {
        return y * 5 + x;       // Linha par (esquerda para direita)
    } else {
        return y * 5 + (4 - x); // Linha ímpar (direita para esquerda)
    }
}

void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program); // Carregar o programa PIO
    np_pio = pio0;                                         // Usar o primeiro bloco PIO

    sm = pio_claim_unused_sm(np_pio, false);              // Tentar usar uma state machine do pio0
    if (sm < 0) {                                         // Se não houver disponível no pio0
        np_pio = pio1;                                    // Mudar para o pio1
        sm = pio_claim_unused_sm(np_pio, true);           // Usar uma state machine do pio1
    }

    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f); // Inicializar state machine para LEDs

    for (uint i = 0; i < LED_COUNT; ++i) {                // Inicializar todos os LEDs como apagados
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;                                    // Definir componente vermelho
    leds[index].G = g;                                    // Definir componente verde
    leds[index].B = b;                                    // Definir componente azul
}

void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i) {                // Iterar sobre todos os LEDs
        npSetLED(i, 0, 0, 0);                             // Definir cor como preta (apagado)
    }
    npWrite();                                            // Atualizar LEDs no hardware
}

void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {                // Iterar sobre todos os LEDs
        pio_sm_put_blocking(np_pio, sm, leds[i].G);       // Enviar componente verde
        pio_sm_put_blocking(np_pio, sm, leds[i].R);       // Enviar componente vermelho
        pio_sm_put_blocking(np_pio, sm, leds[i].B);       // Enviar componente azul
    }
}

void init_buzzer_pwm(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM); // Configura o GPIO como PWM
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_clkdiv(slice_num, 125.0f);     // Define o divisor do clock para 1 MHz
    pwm_set_wrap(slice_num, 1000);        // Define o TOP para frequência de 1 kHz
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(gpio), 0); // Razão cíclica inicial
    pwm_set_enabled(slice_num, true);     // Habilita o PWM
}

void set_buzzer_tone(uint gpio, uint freq) {
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    uint top = 1000000 / freq;            // Calcula o TOP para a frequência desejada
    pwm_set_wrap(slice_num, top);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(gpio), top / 2); // 50% duty cycle
}

void stop_buzzer(uint gpio) {
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(gpio), 0); // Desliga o PWM
}
void set_led_pulse(uint gpio, uint16_t percentage) {
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    uint16_t level = (uint16_t)((WRAP * percentage) / 100);  // Converte 0-100% para 0-WRAP
    pwm_set_gpio_level(gpio, level);
}
void init_hardware(){
    adc_init(); // inicia o adc
    i2c_init(I2C_PORT, 400e3); // inicia o i2c
    // Configuração do rgb
    gpio_init(RED_LED);
    gpio_init(BLUE_LED);
    gpio_init(GREEN_LED);
    gpio_set_dir(RED_LED, GPIO_OUT);
    gpio_set_dir(BLUE_LED, GPIO_OUT);
    gpio_set_dir(GREEN_LED, GPIO_OUT);
    init_led_pwm(RED_LED);
    init_led_pwm(BLUE_LED);
    init_led_pwm(GREEN_LED);
    //configuração dos botões
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    //configuração dos eixos
    adc_gpio_init(X_AXIS);
    adc_gpio_init(Y_AXIS);
    // configuração do display
    gpio_set_function(DISPLAY_SDA, GPIO_FUNC_I2C);
    gpio_set_function(DISPLAY_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(DISPLAY_SDA);
    gpio_pull_up(DISPLAY_SCL);
    //Inicia a matriz de led
    npInit(MATRIX_PIN);
    display_init();
    init_buzzer_pwm(BUZZER_A);
    init_buzzer_pwm(BUZZER_B);
}
void display_init(){
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT); // Inicialização do display
    ssd1306_config(&ssd); // Configura display
    ssd1306_send_data(&ssd); // envia dados para o display

    ssd1306_fill(&ssd, false); // limpa o display
    ssd1306_send_data(&ssd);
}
void reset(){
    watchdog_reboot(0, 0, 0);
}
void init_led_pwm(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice_num, WRAP);
    pwm_set_clkdiv(slice_num, 125.0f); // Divisor de clock para 1 MHz
    pwm_set_enabled(slice_num, true);
}