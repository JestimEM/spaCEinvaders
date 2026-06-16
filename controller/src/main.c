/*
 * spaCEinvaders — Control Fisico (Raspberry Pi Pico)
 * Comunicacion: UART a 115200 baudios
 *
 * Boton 1 (GPIO 14): movimiento — 1 toque=izquierda, 2 toques=derecha
 * Boton 2 (GPIO 15): disparo
 */

#include "pico/stdlib.h"
#include "hardware/uart.h"

#define ID_UART             uart0
#define BAUDIOS             115200
#define PIN_TX_UART         0
#define PIN_RX_UART         1

#define PIN_BTN_MOVIMIENTO  14
#define PIN_BTN_DISPARO     15
#define TIEMPO_REBOTE_MS    50
#define TIEMPO_DOBLE_TAP_MS 300

/* Comandos enviados por UART al cliente */
#define CMD_MOVER_IZQUIERDA  "MOVER_IZQUIERDA\n"
#define CMD_MOVER_DERECHA    "MOVER_DERECHA\n"
#define CMD_DISPARAR         "DISPARAR\n"

static uint32_t ultimo_toque_movimiento = 0;
static int      conteo_toques           = 0;
static uint32_t ultimo_toque_disparo    = 0;

static void inicializar_uart(void) {
    uart_init(ID_UART, BAUDIOS);
    gpio_set_function(PIN_TX_UART, GPIO_FUNC_UART);
    gpio_set_function(PIN_RX_UART, GPIO_FUNC_UART);
}

static void inicializar_botones(void) {
    gpio_init(PIN_BTN_MOVIMIENTO);
    gpio_set_dir(PIN_BTN_MOVIMIENTO, GPIO_IN);
    gpio_pull_up(PIN_BTN_MOVIMIENTO);

    gpio_init(PIN_BTN_DISPARO);
    gpio_set_dir(PIN_BTN_DISPARO, GPIO_IN);
    gpio_pull_up(PIN_BTN_DISPARO);
}

static void manejar_boton_movimiento(void) {
    uint32_t ahora = to_ms_since_boot(get_absolute_time());

    if (!gpio_get(PIN_BTN_MOVIMIENTO) && (ahora - ultimo_toque_movimiento) > TIEMPO_REBOTE_MS) {
        ultimo_toque_movimiento = ahora;
        conteo_toques++;

        if (conteo_toques == 1) {
            /* Esperar para detectar doble toque */
            sleep_ms(TIEMPO_DOBLE_TAP_MS);
            if (conteo_toques == 1) {
                uart_puts(ID_UART, CMD_MOVER_IZQUIERDA);
            }
            conteo_toques = 0;
        } else if (conteo_toques == 2) {
            uart_puts(ID_UART, CMD_MOVER_DERECHA);
            conteo_toques = 0;
        }
    }
}

static void manejar_boton_disparo(void) {
    uint32_t ahora = to_ms_since_boot(get_absolute_time());

    if (!gpio_get(PIN_BTN_DISPARO) && (ahora - ultimo_toque_disparo) > TIEMPO_REBOTE_MS) {
        ultimo_toque_disparo = ahora;
        uart_puts(ID_UART, CMD_DISPARAR);
    }
}

int main(void) {
    stdio_init_all();
    inicializar_uart();
    inicializar_botones();

    while (true) {
        manejar_boton_movimiento();
        manejar_boton_disparo();
        sleep_ms(10);
    }

    return 0;
}
