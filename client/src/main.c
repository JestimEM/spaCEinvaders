#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
#define THREAD_FUNC      DWORD WINAPI
#define thread_sleep(ms) Sleep(ms)
#define MUTEX_LOCK()     EnterCriticalSection(&estado_mutex)
#define MUTEX_UNLOCK()   LeaveCriticalSection(&estado_mutex)
typedef HANDLE thread_t;
#else
#include <pthread.h>
#include <unistd.h>
#define THREAD_FUNC      void *
#define thread_sleep(ms) usleep((ms) * 1000)
#define MUTEX_LOCK()     pthread_mutex_lock(&estado_mutex)
#define MUTEX_UNLOCK()   pthread_mutex_unlock(&estado_mutex)
typedef pthread_t thread_t;
#endif

#include "raylib.h"

#include <string.h>
#include <stdio.h>
#include "../include/constants.h"
#include "../include/game_state.h"
#include "../include/network.h"
#include "../include/render.h"
#include "../include/json_utils.h"

/* ---- Estado global compartido con el hilo de red ---- */
static EstadoJuego estado;
static volatile int detener_red = 0;
static int sockfd_global;   /* el hilo lee este fd al arrancar */

#ifdef _WIN32
static CRITICAL_SECTION estado_mutex;
#else
static pthread_mutex_t estado_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* ================================================================
   Hilo de red — lee mensajes del servidor y los aplica al estado
   ================================================================ */
static THREAD_FUNC hilo_red_fn(void *arg) {
    int sockfd = *(int *)arg;
    char buf[TAMANO_BUFFER];
    while (!detener_red) {
        int n = red_recibir(sockfd, buf, sizeof(buf));
        if (n < 0 || n == 0) {
            if (!detener_red) {
                MUTEX_LOCK();
                estado.fin_juego = 1;
                MUTEX_UNLOCK();
            }
            break;
        }
        Mensaje msg;
        red_parsear_mensaje(buf, &msg);
        MUTEX_LOCK();
        red_aplicar_mensaje(&msg, &estado);
        MUTEX_UNLOCK();
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static thread_t arrancar_hilo_red(void) {
    detener_red = 0;
    thread_t hilo;
#ifdef _WIN32
    hilo = CreateThread(NULL, 0, hilo_red_fn, &sockfd_global, 0, NULL);
#else
    pthread_create(&hilo, NULL, hilo_red_fn, &sockfd_global);
#endif
    return hilo;
}

/* Señala al hilo que pare (via shutdown del socket) y espera su fin */
static void parar_hilo_red(thread_t hilo, int sockfd) {
    detener_red = 1;
    red_cerrar_socket(sockfd);   /* shutdown + close — desbloquea recv() */
#ifdef _WIN32
    WaitForSingleObject(hilo, 2000);
    CloseHandle(hilo);
#else
    pthread_join(hilo, NULL);
#endif
    red_reset_buffer();
}

/* ================================================================
   Modo Jugador — conecta al puerto 8080, puede reiniciar
   ================================================================ */
static void modo_jugador(void) {
    render_cargar_texturas();   /* carga una sola vez — sirve para todos los reinicios */
    for (;;) {  /* bucle de reinicio */
        BeginDrawing(); ClearBackground(BLACK);
        DrawText("Conectando al servidor...",
                 (SCREEN_W - MeasureText("Conectando al servidor...", 20)) / 2,
                 SCREEN_H / 2 - 10, 20, YELLOW);
        EndDrawing();
        int sockfd = red_conectar(IP_SERVIDOR, PUERTO_SERVIDOR);
        if (sockfd < 0) {
            BeginDrawing(); ClearBackground(BLACK);
            DrawText("Error: no se pudo conectar al servidor (jugadores)",
                     40, SCREEN_H / 2, 20, RED);
            EndDrawing();
            thread_sleep(2000);
            return;
        }

        estado_juego_inicializar(&estado);
        sockfd_global = sockfd;
        thread_t hilo = arrancar_hilo_red();

        int accion = -1;   /* -1=salir/ESC  0=salir desde fin  1=reiniciar */

        while (!WindowShouldClose()) {
            MUTEX_LOCK();
            EstadoJuego snap = estado;
            MUTEX_UNLOCK();

            if (snap.fin_juego) {
                accion = render_pantalla_fin(snap.jugador.puntuacion,
                                             snap.jugador.vidas > 0);
                break;
            }

            /* Input de juego */
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
                red_enviar(sockfd, "{\"tipo\":\"MOVER_CANON\",\"direccion\":\"IZQUIERDA\"}");
            else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
                red_enviar(sockfd, "{\"tipo\":\"MOVER_CANON\",\"direccion\":\"DERECHA\"}");
            if (IsKeyPressed(KEY_SPACE) && !snap.bala_activa)
                red_enviar(sockfd, "{\"tipo\":\"DISPARO\"}");
            if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_ESCAPE)) break;

            render_frame(&snap, 0, 0);
        }

        parar_hilo_red(hilo, sockfd);

        if (accion == 1 && !WindowShouldClose()) {
            /* Reiniciar: el socket ya fue cerrado por parar_hilo_red,
               volvemos al tope del for(;;) para reconectar */
            continue;
        }
        break;
    }
}

/* ================================================================
   Modo Observador — conecta al puerto 8081, selecciona sesión
   ================================================================ */
static void modo_observador(void) {
    BeginDrawing(); ClearBackground(BLACK);
    DrawText("Conectando como observador...",
             (SCREEN_W - MeasureText("Conectando como observador...", 20)) / 2,
             SCREEN_H / 2 - 10, 20, SKYBLUE);
    EndDrawing();
    int sockfd = red_conectar(IP_SERVIDOR, PUERTO_ESPECTADOR);
    if (sockfd < 0) {
        BeginDrawing(); ClearBackground(BLACK);
        DrawText("Error: no se pudo conectar al servidor (espectadores)",
                 40, SCREEN_H / 2, 20, RED);
        EndDrawing();
        thread_sleep(2000);
        return;
    }

    /* El servidor envía SESIONES inmediatamente al conectarse */
    int   ids[16]; int n_ids = 0;
    char  buf[TAMANO_BUFFER];
    int r = red_recibir(sockfd, buf, sizeof(buf));
    if (r > 0) {
        Mensaje msg;
        red_parsear_mensaje(buf, &msg);
        if (strcmp(msg.tipo, "SESIONES") == 0)
            n_ids = json_int_array(msg.carga, "ids", ids, 16);
    }

    /* Mostrar lista y esperar selección */
    int sesion_id = render_lista_sesiones(ids, n_ids);
    if (sesion_id < 0 || WindowShouldClose()) {
        red_desconectar(sockfd);
        return;
    }

    /* Enviar selección al servidor */
    char seleccion[64];
    snprintf(seleccion, sizeof(seleccion),
             "{\"tipo\":\"SELECCIONAR_SESION\",\"id\":%d}", sesion_id);
    red_enviar(sockfd, seleccion);

    render_cargar_texturas();   /* carga perezosa — no bloquea el menú */
    /* Arrancar hilo de red y entrar en modo espectador */
    estado_juego_inicializar(&estado);
    sockfd_global = sockfd;
    thread_t hilo = arrancar_hilo_red();

    while (!WindowShouldClose()) {
        MUTEX_LOCK();
        EstadoJuego snap = estado;
        MUTEX_UNLOCK();

        if (snap.fin_juego || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q))
            break;
        render_frame(&snap, 1, sesion_id);
    }

    parar_hilo_red(hilo, sockfd);
    /* red_desconectar omitido: parar_hilo_red ya cerró el socket via red_cerrar_socket */
}

/* ================================================================
   main
   ================================================================ */
int main(void) {
    render_inicializar();

#ifdef _WIN32
    InitializeCriticalSection(&estado_mutex);
#endif

    int rol = render_menu();   /* 0=Jugador  1=Observador  -1=salir */
    if (!WindowShouldClose() && rol >= 0) {
        if (rol == 0) modo_jugador();
        else          modo_observador();
    }

#ifdef _WIN32
    DeleteCriticalSection(&estado_mutex);
#endif
    render_destruir();
    return 0;
}
