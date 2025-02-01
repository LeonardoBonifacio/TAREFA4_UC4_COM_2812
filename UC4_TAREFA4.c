#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/bootrom.h"


// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2812.pio.h"

// Definição do número de LEDs e pinos.
#define LED_COUNT 25
#define MATRIZ_LED_PIN 7
#define BUTTON_A 5
#define BUTTON_B 6
#define BUTTON_JOYSTICK 22 
#define LED_RED 13
#define DEBOUNCE_DELAY_MS 500  // Tempo de debounce em milissegundos 


// Váriaveis volatile para indicar ao compilador que elas serão alteradas por eventos externos

// Variável usada para saber em quanto está o contador que será mostrado na matriz de leds
// Caso a interrupção seja acionada pelo botão B seja acionada esta variável é decrementada
// E o contrario acontece caso a interrupção seja acionada pelo botão A
static volatile int incrementa_ou_decrementa_led = 0;

static volatile bool atualiza_leds = false;  // Flag para sinalizar atualização da matriz
static volatile uint32_t ultima_alteracao_led_vermelho = 0;  // Para controle de tempo do LED vermelho

volatile uint32_t ultimo_tempo_button_a = 0;  // Para armazenar o tempo da última interrupção acionada pelo bottão A
volatile uint32_t ultimo_tempo_button_b = 0;  // Para armazenar o tempo da última interrupção acionada pelo bottão B





// Buffer para armazenar quais LEDs estão ligados matriz 5x5 formando numero 0
bool numero_0[LED_COUNT] = {
    0, 1, 1, 1, 1, 
    1, 0, 0, 1, 0, 
    0, 1, 0, 0, 1, 
    1, 0, 0, 1, 0, 
    0, 1, 1, 1, 1
};


// Buffer para armazenar quais LEDs estão ligados matriz 5x5 formando numero 1
bool numero_1[LED_COUNT] = {
    0, 0, 1, 0, 0, 
    0, 0, 1, 0, 0, 
    0, 0, 1, 0, 1, 
    0, 1, 1, 0, 0, 
    0, 0, 1, 0, 0
};

// Buffer para armazenar quais LEDs estão ligados matriz 5x5 formando numero 2
bool numero_2[LED_COUNT] = {
    0, 1, 1, 1, 1, 
    1, 0, 0, 0, 0, 
    0, 1, 1, 1, 1, 
    0, 0, 0, 1, 0, 
    0, 1, 1, 1, 1
};

// Buffer para armazenar quais LEDs estão ligados matriz 5x5 formando numero 3
bool numero_3[LED_COUNT] = {
    0, 1, 1, 1, 1, 
    0, 0, 0, 1, 0, 
    0, 1, 1, 1, 1, 
    0, 0, 0, 1, 0, 
    0, 1, 1, 1, 1
};


// Buffer para armazenar quais LEDs estão ligados matriz 5x5 formando numero 4
bool numero_4[LED_COUNT] = {
    0, 1, 0, 0, 0, 
    0, 0, 0, 1, 0, 
    0, 1, 1, 1, 1, 
    1, 0, 0, 1, 0, 
    0, 1, 0, 0, 1
};


// Buffer para armazenar quais LEDs estão ligados matriz 5x5 formando numero 5
bool numero_5[LED_COUNT] = {
    0, 1, 1, 1, 1, 
    0, 0, 0, 1, 0, 
    0, 1, 1, 1, 1, 
    1, 0, 0, 0, 0, 
    0, 1, 1, 1, 1
};


// Buffer para armazenar quais LEDs estão ligados matriz 5x5 formando numero 6
bool numero_6[LED_COUNT] = {
    0, 1, 1, 1, 1, 
    1, 0, 0, 1, 0, 
    0, 1, 1, 1, 1, 
    1, 0, 0, 0, 0, 
    0, 1, 1, 1, 1
};

// Buffer para armazenar quais LEDs estão ligados matriz 5x5 formando numero 7
bool numero_7[LED_COUNT] = {
    0, 1, 0, 0, 0, 
    0, 0, 0, 1, 0, 
    0, 1, 0, 0, 1, 
    1, 0, 0, 1, 0, 
    0, 1, 1, 1, 1
};

// Buffer para armazenar quais LEDs estão ligados matriz 5x5 formando numero 8
bool numero_8[LED_COUNT] = {
    0, 1, 1, 1, 1, 
    1, 0, 0, 1, 0, 
    0, 1, 1, 1, 1, 
    1, 0, 0, 1, 0, 
    0, 1, 1, 1, 1
};

// Buffer para armazenar quais LEDs estão ligados matriz 5x5 formando numero 9
bool numero_9[LED_COUNT] = {
    0, 1, 0, 0, 0, 
    0, 0, 0, 1, 0, 
    0, 1, 1, 1, 1, 
    1, 0, 0, 1, 0, 
    0, 1, 1, 1, 1
};



static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}


void set_one_led(uint8_t r, uint8_t g, uint8_t b, bool numero_a_ser_desenhado[])
{
    // Define a cor com base nos parâmetros fornecidos
    uint32_t color = urgb_u32(r, g, b);

    // Define todos os LEDs com a cor especificada
    for (int i = 0; i < LED_COUNT; i++)
    {
        if (numero_a_ser_desenhado[i])
        {
            put_pixel(color); // Liga o LED com um no buffer
        }
        else
        {
            put_pixel(0);  // Desliga os LEDs com zero no buffer
        }
    }
}

// Inicializa as gpio referentes aos botões e leds, alem de colocar os 3 botões em pull_up 
void inicializar_leds_e_botoes(){
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A,GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B,GPIO_IN);
    gpio_pull_up(BUTTON_B);

    gpio_init(BUTTON_JOYSTICK);
    gpio_set_dir(BUTTON_JOYSTICK,GPIO_IN);
    gpio_pull_up(BUTTON_JOYSTICK);

    gpio_init(LED_RED);
    gpio_set_dir(LED_RED,GPIO_OUT);
    gpio_put(LED_RED,false);
}



// Desenha baseado em que número o contador está
void mostra_numero_baseado_no_contador(){
    switch (incrementa_ou_decrementa_led) 
    {
    case 0:
        set_one_led(255,255,255,numero_0);// Branco
        break;
    case 1:
        set_one_led(255,0,0,numero_1); // Vermelho
        break;
    case 2:
        set_one_led(255,127,0,numero_2); // Amarelo
        break;
    case 3:
        set_one_led(169,169,169,numero_3); // Cinza
        break;
    case 4:
        set_one_led(0,255,0,numero_4); // Verde
        break;
    case 5:
        set_one_led(0,0,255,numero_5); // Azul
        break;
    case 6:
        set_one_led(255,140,0,numero_6); // Laranja
        break;
    case 7:
        set_one_led(139,0,255,numero_7); // Roxo
        break;
    case 8:
        set_one_led(139,69,19,numero_8); // Branco
        break;
    case 9:
        set_one_led(255,20,147,numero_9); // Rosa
        break;
    }
}


// Função que captura a interrupção global e baseada em qual gpio mandou a interrupção ela ativa a lógica correspondente
static void gpio_irq_handler(uint gpio, uint32_t events) {
    if (gpio == BUTTON_A) {
        // Interrupção que trata de aumentar o número mostrado pela matriz de leds
        uint32_t tempo_atual = time_us_32() / 1000;  // Obtém o tempo atual em milissegundos e o armazena

        // Se o tempo passado for menor que o atraso  de debounce(500ms) retorne imediatamente
        if (tempo_atual - ultimo_tempo_button_a < DEBOUNCE_DELAY_MS) return;

        // O tempo atual corresponde ao último tempo que o botão foi pressionado, ja que ele passou pela verificação acima
        ultimo_tempo_button_a = tempo_atual;

        // impede do contador de ficar maior que 9 e aumenta ele para que seja atualizado no loop principal
        if (incrementa_ou_decrementa_led + 1 < 10) incrementa_ou_decrementa_led++;
        
        // Muda o estado da flag para que ocorra a atualização dos leds apos o contador ser incrementado
        atualiza_leds = true;

    } else if (gpio == BUTTON_B) {
        // Interrupção que trata de diminuir o número mostrado pela matriz de leds
        uint32_t tempo_atual = time_us_32() / 1000; // Obtém o tempo atual em milissegundos e o armazena

        // Se o tempo passado for menor que o atraso  de debounce(500ms) retorne imediatamente
        if (tempo_atual - ultimo_tempo_button_b < DEBOUNCE_DELAY_MS) return;

        // O tempo atual corresponde ao último tempo que o botão foi pressionado, ja que ele passou pela verificação acima
        ultimo_tempo_button_b = tempo_atual;

        // impede do contador de ficar menor que 0 e diminui ele para que seja atualizado no loop principal
        if (incrementa_ou_decrementa_led - 1 >= 0) incrementa_ou_decrementa_led--;

        // Muda o estado da flag para que ocorra a atualização dos leds apos o contador ser decrementado
        atualiza_leds = true;

    } else if (gpio == BUTTON_JOYSTICK) {
        // Interrupção para habilitar o modo de gravação do microcontrolador
        reset_usb_boot(0,0);
    }
}


int main() {
    inicializar_leds_e_botoes();
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio, sm, offset, MATRIZ_LED_PIN, 800000, false);

    // Registra interrupções para todos os botões
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_JOYSTICK, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while (true) {
        uint32_t tempo_atual = time_us_32(); // Captura o tempo desde que o programa foi iniciado em microssegundos

        // Atualiza matriz de LEDs se necessário
        if (atualiza_leds) {
            mostra_numero_baseado_no_contador();
            atualiza_leds = false;  // Reseta flag
        }

        // Pisca o LED vermelho 5 vezes por segundo
        // Cada ciclo "Custa" 200 ms , 100ms do led ligado e 100ms dele desligado, 
        // totalizando 5 ciclos por segundo ou seja ele pisca 5 vezes por segundo
        // Calcula o tempo que passou desde a ultima vez que o led alterou seu estado e 
        // verifica se pelo menos 100ms se passaram desde a ultima alteração
        if (tempo_atual - ultima_alteracao_led_vermelho >= 100000) {  // 100ms = 100000µs
            gpio_put(LED_RED, !gpio_get(LED_RED));  // Inverte estado
            ultima_alteracao_led_vermelho = tempo_atual;
        }
    }
}

