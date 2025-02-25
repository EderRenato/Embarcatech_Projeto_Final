#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2818b.pio.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

#define LED_COUNT 25                // Numero de leds da matriz
#define MATRIX_PIN 7                // Pino da matriz de LEDs
#define I2C_PORT i2c1               // Porta i2c1
#define DISPLAY_SDA 14              // Pino do SDA
#define DISPLAY_SCL 15              // Pino do SCL
#define ENDERECO 0x3C               // Endereço do display
#define DIVISOR_CLOCK_PWM 125.0f    // Divisor do clock do pwm
#define VALOR_WRAP_PWM 1000         // Valor do wrap do pwm

const uint GREEN_LED = 11;          // Pino do led verde
const uint BLUE_LED = 12;           // Pino do led azul
const uint RED_LED = 13;            // Pino do led vermelho
const uint X_AXIS = 26;             //eixo x do joystick
const uint Y_AXIS = 27;             //eixo y do joystick
const uint BUZZER_A = 21;           // Pino do buzzer A
const uint BUZZER_B = 10;           // Pino do buzzer B
const uint BUTTON_A = 5;            // Pino do botão A
const uint BUTTON_B = 6;            // Pino do botão B
const uint DEBOUNCE_DELAY = 150;   // Tempo de debounce para os botões
bool alarme_ativo = false; // Indica se o alarme está ativo
uint32_t alarme_inicio = 0; // Tempo de início do alarme
const uint32_t ALARME_DURACAO_MS = 1000; // Duração do alarme em milissegundos

uint UMIDADE = 0; // Variável para armazenar a umidade
uint PH = 0; // Variavel para armazenar o ph
uint modo = 0; // modo de operação 'Hortaliças', 1 para 'Cactus', 2 para 'orquidea'
bool irrigacao = false; // Variavel para armazenar o estado da irrigação

uint UMIDADE_MIN[3] = {40, 10, 60}; // Umidade minima para os três modos de funcionamento
float PH_MIN[3] = {6.0, 5.5, 5.0}; // pH minimo para os três modos
float PH_MAX[3] = {7.0, 6.5, 6.0}; // pH maximo para os tres modos

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
// funcção para inicializar os buzzers como pwm
void init_buzzer_pwm(uint gpio); 
// função para inicializar os leds como pwm
void init_led_pwm(uint gpio); 
// leitura do adc
uint16_t read_adc(uint channel); 
// Configures the buzzer to emit a tone at the specified frequency.
void set_buzzer_tone(uint gpio, uint freq);
// Para o pwm
void stop_pwm(uint gpio);
// envio de dados do pwm para o led
void set_led_pulse(uint gpio, uint16_t percentage);
// função de interrupção dos botões
void buttons_callback(uint gpio, uint32_t events);
//função para inicializar o display
void display_init();
//função para inicializar todos os componentes
void init_hardware();
//função para acionar o alarme
void alarm(); 
//função para exibir a barra de progresso de irrigacao
void modo_de_operacao();

int main()
{
    stdio_init_all();
    init_hardware();
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, buttons_callback);
    gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);
    while (true) {
        UMIDADE = read_adc(Y_AXIS);
        PH = read_adc(X_AXIS);
        UMIDADE = (UMIDADE * 100) / 4095;
        PH = (PH * 14) / 4095;

        if (alarme_ativo && (to_ms_since_boot(get_absolute_time()) - alarme_inicio) >= ALARME_DURACAO_MS) {
            //desativa o alarme se já tiver passado mais que 1 segundo
            alarme_ativo = false;
            stop_pwm(RED_LED);
            stop_pwm(GREEN_LED);
            stop_pwm(BLUE_LED);
            stop_pwm(BUZZER_A);
            stop_pwm(BUZZER_B);
        }
        // Limpa o display
        ssd1306_fill(&ssd, false);
        // Exibe o valor da umidade
        char umidade_str[20];
        snprintf(umidade_str, sizeof(umidade_str), "Umidade: %u%%", UMIDADE);
        ssd1306_draw_string(&ssd, umidade_str, 10, 0);
        // Exibe o valor do pH
        char ph_str[20];
        snprintf(ph_str, sizeof(ph_str), "pH: %.1f", (float)PH);
        ssd1306_draw_string(&ssd, ph_str, 10, 10);
        // Exibe o modo atual
        char modo_str[20];
        const char *modos[] = {"Hortalicas", "Cactus", "Orquidea"};
        snprintf(modo_str, sizeof(modo_str), "Modo: %s", modos[modo]);
        ssd1306_draw_string(&ssd, modo_str, 10, 20);
        // Exibe o status da irrigação
        if (irrigacao) {
            ssd1306_draw_string(&ssd, "Irrigando...", 10, 50);
        } else {
            ssd1306_draw_string(&ssd, "Irrigacao OK", 10, 50);
        }
        // Verifica e exibe alertas de pH
        if (PH < PH_MIN[modo]) {
            alarm();
            ssd1306_draw_string(&ssd, "pH baixo Ajuste pH", 10, 40);
        } else if (PH > PH_MAX[modo]) {
            alarm();
            ssd1306_draw_string(&ssd, "pH alto Ajuste pH", 10, 40);
        } else {
            ssd1306_draw_string(&ssd, "pH ok!", 10, 40);
        }
        // Atualiza o display uma única vez
        ssd1306_send_data(&ssd);
        // Altera os leds rgb de acordo com o nivel de umidade
        if (UMIDADE < UMIDADE_MIN[modo]) {
            set_led_pulse(RED_LED, 100-UMIDADE);
            stop_pwm(GREEN_LED);
            stop_pwm(BLUE_LED);
        } else if(UMIDADE < UMIDADE_MIN[modo] + 20) {
            set_led_pulse(RED_LED, 100-UMIDADE);
            set_led_pulse(GREEN_LED, 100-UMIDADE);
            stop_pwm(BLUE_LED);
        } else if (UMIDADE < UMIDADE_MIN[modo] + 35) {
            stop_pwm(RED_LED);
            set_led_pulse(GREEN_LED, 100-UMIDADE);
            set_led_pulse(BLUE_LED, UMIDADE);
        } else {
            stop_pwm(GREEN_LED);
            set_led_pulse(BLUE_LED, UMIDADE);
        }
        // Controle de irrigação
        if (!irrigacao && UMIDADE < UMIDADE_MIN[modo]) {
            alarm();
            irrigacao = true;
        }else if(irrigacao && UMIDADE == UMIDADE_MIN[modo]+40) {
            irrigacao = false;
        }
        // mostra na matriz de led o modo de operação
        modo_de_operacao();
        // aguarda 100ms antes de rodar o loop novamente
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
void init_pwm(uint gpio, float clkdiv, uint wrap) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_clkdiv(slice_num, clkdiv);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_enabled(slice_num, true);
}
void set_buzzer_tone(uint gpio, uint freq) {
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    uint top = 1000000 / freq;            // Calcula o TOP para a frequência desejada
    pwm_set_wrap(slice_num, top);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(gpio), top / 2); // 50% duty cycle
}
void stop_pwm(uint gpio) {
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(gpio), 0); // Desliga o PWM
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
    init_pwm(RED_LED, DIVISOR_CLOCK_PWM, VALOR_WRAP_PWM);
    init_pwm(BLUE_LED, DIVISOR_CLOCK_PWM, VALOR_WRAP_PWM);
    init_pwm(GREEN_LED, DIVISOR_CLOCK_PWM, VALOR_WRAP_PWM);
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
    display_init();
    //Inicia a matriz de led
    npInit(MATRIX_PIN);
    npClear();
    npWrite();
    // Inicia os buzzers
    init_pwm(BUZZER_A, DIVISOR_CLOCK_PWM, VALOR_WRAP_PWM);
    init_pwm(BUZZER_B, DIVISOR_CLOCK_PWM, VALOR_WRAP_PWM);
}
void display_init(){
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT); // Inicialização do display
    ssd1306_config(&ssd); // Configura display
    ssd1306_send_data(&ssd); // envia dados para o display

    ssd1306_fill(&ssd, false); // limpa o display
    ssd1306_send_data(&ssd);
}
void set_led_pulse(uint gpio, uint16_t percentage) {
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    uint16_t level = (uint16_t)((VALOR_WRAP_PWM * percentage) / 100);  // Converte 0-100% para 0-WRAP
    printf("Level: %u \nPorcentagem: %u\n", level, percentage);  // Depuração
    pwm_set_gpio_level(gpio, level);
}
uint16_t read_adc(uint channel) {
    if (channel < 26 || channel > 28) return 0; // Verifica se o canal é válido
    adc_select_input(channel - 26); // Ajusta o canal (26 → 0, 27 → 1, 28 → 2)
    uint32_t sum = 0;
    const int samples = 10;  // Coleta 10 amostras e faz a média
    for (int i = 0; i < samples; i++) {
        sum += adc_read();
        sleep_us(500);  // Pequeno atraso para suavizar leituras
    }
    return sum / samples;
}
void buttons_callback(uint gpio, uint32_t events){
    static uint32_t button_a_last_time = 0;
    static uint32_t button_b_last_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (gpio == BUTTON_A && (current_time - button_a_last_time) > DEBOUNCE_DELAY) {
        button_a_last_time = current_time;
        modo = (modo + 1) % 3; // Alterna entre 0, 1 e 2
    } else if (gpio == BUTTON_B && (current_time - button_b_last_time) > DEBOUNCE_DELAY) {
        button_b_last_time = current_time;
        stop_pwm(BLUE_LED);
        irrigacao = false; // Reseta o estado da irrigação
    }
}
void alarm() {
    if (!alarme_ativo) {
        alarme_ativo = true;
        alarme_inicio = to_ms_since_boot(get_absolute_time());

        // Ativa o buzzer e os LEDs
        set_buzzer_tone(BUZZER_A, 1000);
        set_buzzer_tone(BUZZER_B, 1000);
        set_led_pulse(RED_LED, 100);
        set_led_pulse(GREEN_LED, 100);
        set_led_pulse(BLUE_LED, 100);
    }
}
void modo_de_operacao() {
    int matriz[3][5][5][3] = {
        {
            {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {18, 131, 0}},
            {{0, 0, 0}, {0, 0, 0}, {255, 178, 0}, {255, 178, 0}, {0, 0, 0}},
            {{0, 0, 0}, {255, 178, 0}, {255, 178, 0}, {255, 178, 0}, {0, 0, 0}},
            {{255, 178, 0}, {255, 178, 0}, {255, 178, 0}, {0, 0, 0}, {0, 0, 0}},
            {{255, 178, 0}, {255, 178, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}
        },
        {
            {{18, 131, 0}, {0, 0, 0}, {18, 131, 0}, {0, 0, 0}, {18, 131, 0}},
            {{18, 131, 0}, {0, 0, 0}, {18, 131, 0}, {0, 0, 0}, {18, 131, 0}},
            {{18, 131, 0}, {18, 131, 0}, {18, 131, 0}, {0, 0, 0}, {18, 131, 0}},
            {{0, 0, 0}, {0, 0, 0}, {18, 131, 0}, {18, 131, 0}, {18, 131, 0}},
            {{0, 0, 0}, {0, 0, 0}, {18, 131, 0}, {0, 0, 0}, {0, 0, 0}}
        },
        {
            {{0, 0, 0}, {138, 0, 211}, {138, 0, 211}, {138, 0, 211}, {0, 0, 0}},
            {{138, 0, 211}, {255, 50, 246}, {255, 50, 246}, {255, 50, 246}, {138, 0, 211}},
            {{138, 0, 211}, {255, 50, 246}, {165, 162, 0}, {255, 50, 246}, {138, 0, 211}},
            {{138, 0, 211}, {255, 50, 246}, {255, 50, 246}, {255, 50, 246}, {138, 0, 211}},
            {{0, 0, 0}, {138, 0, 211}, {138, 0, 211}, {138, 0, 211}, {0, 0, 0}}
        }};
    for (int linha = 0; linha < 5; linha++) {
        for (int coluna = 0; coluna < 5; coluna++) {
            int posicao = getIndex(linha, coluna);
            npSetLED(posicao, matriz[modo][coluna][linha][0], matriz[modo][coluna][linha][1], matriz[modo][coluna][linha][2]);
        }
    }
    npWrite(); // Atualizar LEDs no hardware
}