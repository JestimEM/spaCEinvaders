#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOGDI
#  define NOUSER
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
   typedef SOCKET sock_t;
#  define SOCK_CLOSE(s) closesocket(s)
#  define SOCK_INVALID  INVALID_SOCKET
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <unistd.h>
   typedef int sock_t;
#  define SOCK_CLOSE(s) close(s)
#  define SOCK_INVALID  (-1)
#endif

#include "../include/network.h"
#include "../include/game_state.h"
#include "../include/constants.h"
#include "../include/json_utils.h"

/* Ring buffer compartido (file scope para poder resetearlo en reconexión) */
static char rbuf[TAMANO_BUFFER * 2];
static int  rbuf_end = 0;

void red_reset_buffer(void) {
    rbuf_end = 0;
}

#ifdef _WIN32
static int wsa_done = 0;
#endif

int red_conectar(const char *ip, int puerto) {
#ifdef _WIN32
    if (!wsa_done) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return -1;
        wsa_done = 1;
    }
#endif
    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == SOCK_INVALID) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((unsigned short)puerto);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        SOCK_CLOSE(s);
        return -1;
    }

    /* Deshabilitar Nagle: cada send() sale inmediatamente sin esperar ACK */
    int flag = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, sizeof(flag));

    return (int)s;
}

void red_desconectar(int sockfd) {
    SOCK_CLOSE((sock_t)sockfd);
}

void red_cerrar_socket(int sockfd) {
    /* shutdown interrumpe recv() en el hilo de red; close libera el fd */
#ifdef _WIN32
    shutdown((SOCKET)sockfd, SD_BOTH);
    closesocket((SOCKET)sockfd);
#else
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
#endif
}

int red_enviar(int sockfd, const char *json) {
    char buf[TAMANO_BUFFER + 2];
    int  n = snprintf(buf, sizeof(buf), "%s\n", json);
    if (n <= 0) return -1;
#ifdef _WIN32
    int sent = send((sock_t)sockfd, buf, n, 0);
#else
    int sent = (int)send((sock_t)sockfd, buf, (size_t)n, 0);
#endif
    return (sent < 0) ? -1 : 0;
}

int red_recibir(int sockfd, char *buffer, int largo_maximo) {
    /* Primero buscar \n en datos ya acumulados de llamadas anteriores */
    for (int i = 0; i < rbuf_end; i++) {
        if (rbuf[i] == '\n') {
            int len = (i < largo_maximo - 1) ? i : largo_maximo - 1;
            memcpy(buffer, rbuf, len);
            buffer[len] = '\0';
            int resto = rbuf_end - (i + 1);
            if (resto > 0) memmove(rbuf, rbuf + i + 1, resto);
            rbuf_end = resto;
            return len;
        }
    }

    /* Leer en bloques hasta encontrar \n — 250x menos syscalls que byte a byte */
    while (rbuf_end < (int)sizeof(rbuf) - 1) {
        int espacio = (int)sizeof(rbuf) - 1 - rbuf_end;
        int chunk = (espacio > 512) ? 512 : espacio;
#ifdef _WIN32
        int r = recv((sock_t)sockfd, rbuf + rbuf_end, chunk, 0);
#else
        int r = (int)recv((sock_t)sockfd, rbuf + rbuf_end, (size_t)chunk, 0);
#endif
        if (r < 0) { buffer[0] = '\0'; return -1; }
        if (r == 0) break;
        for (int k = rbuf_end; k < rbuf_end + r; k++) {
            if (rbuf[k] == '\n') {
                int len = (k < largo_maximo - 1) ? k : largo_maximo - 1;
                memcpy(buffer, rbuf, len);
                buffer[len] = '\0';
                int resto = rbuf_end + r - (k + 1);
                if (resto > 0) memmove(rbuf, rbuf + k + 1, resto);
                rbuf_end = resto;
                return len;
            }
        }
        rbuf_end += r;
    }
    buffer[0] = '\0';
    return 0;
}

void red_parsear_mensaje(const char *json, Mensaje *msg) {
    json_str(json, "tipo", msg->tipo, sizeof(msg->tipo));
    strncpy(msg->carga, json, sizeof(msg->carga) - 1);
    msg->carga[sizeof(msg->carga) - 1] = '\0';
}

void red_aplicar_mensaje(const Mensaje *msg, EstadoJuego *ej) {
    const char *j = msg->carga;

    if (strcmp(msg->tipo, MSG_ESTADO_JUEGO) == 0) {
        ej->jugador.vidas              = json_int(j,   "vidas",      ej->jugador.vidas);
        ej->jugador.puntuacion         = json_int(j,   "puntuacion", ej->jugador.puntuacion);
        ej->velocidad_extraterrestre   = json_float(j, "velocidad",  ej->velocidad_extraterrestre);
        ej->ronda                      = json_int(j,   "ronda",      ej->ronda);
        ej->jugador.posicion_canon_x   = json_int(j,   "canonX",     ej->jugador.posicion_canon_x);

        /* Bala del jugador (server-side) */
        ej->bala_activa = json_int(j, "balaActiva", 0);
        if (ej->bala_activa) {
            ej->bala_x = json_int(j, "balaX", ej->bala_x);
            ej->bala_y = json_int(j, "balaY", ej->bala_y);
        }

        /* Aliens — array plano: [id,x,y,tipo_int,pts, id,x,y,tipo_int,pts, ...] */
        {
            int vals[MAX_EXTRATERR * 5];
            int nv = json_int_array(j, "aliens", vals, MAX_EXTRATERR * 5);
            int cnt = 0;
            for (int ai = 0; ai + 4 < nv; ai += 5, cnt++) {
                ej->extraterrestres[cnt].id           = vals[ai];
                ej->extraterrestres[cnt].x            = vals[ai + 1];
                ej->extraterrestres[cnt].y            = vals[ai + 2];
                ej->extraterrestres[cnt].tipo         = vals[ai + 3];
                ej->extraterrestres[cnt].puntos       = vals[ai + 4];
                ej->extraterrestres[cnt].vivo         = 1;
                ej->extraterrestres[cnt].golpe_visual = 0;
            }
            ej->cantidad_extraterrestres = cnt;
        }

        /* Bunkers */
        int bvals[NUM_BUNKERS];
        int nb = json_int_array(j, "bunkers", bvals, NUM_BUNKERS);
        for (int i = 0; i < NUM_BUNKERS; i++)
            ej->bunkers[i].porcentaje_salud = (i < nb) ? bvals[i] : 100;

        /* OVNI — array plano: [activo, x, pts] */
        {
            int ov[3] = {0, 0, 0};
            json_int_array(j, "ovni", ov, 3);
            ej->ovni.activo = ov[0];
            if (ej->ovni.activo) {
                ej->ovni.x      = ov[1];
                ej->ovni.puntos = ov[2];
            }
        }

        /* Balas alien — array plano: [x,y, x,y, ...] */
        {
            int ba_vals[MAX_BALAS_ALIEN * 2];
            int nb = json_int_array(j, "balasAlien", ba_vals, MAX_BALAS_ALIEN * 2);
            int ba_cnt = 0;
            for (int bi = 0; bi + 1 < nb; bi += 2, ba_cnt++) {
                ej->balas_alien[ba_cnt].x      = ba_vals[bi];
                ej->balas_alien[ba_cnt].y      = ba_vals[bi + 1];
                ej->balas_alien[ba_cnt].activa = 1;
            }
            ej->cantidad_balas_alien = ba_cnt;
        }

        ej->ronda_completa = 0;
        return;
    }

    if (strcmp(msg->tipo, MSG_EXTRATERR_ELIM) == 0) {
        int id = json_int(j, "id", -1);
        for (int i = 0; i < ej->cantidad_extraterrestres; i++) {
            if (ej->extraterrestres[i].id == id) {
                ej->extraterrestres[i].vivo         = 0;
                ej->extraterrestres[i].golpe_visual = 0;
                break;
            }
        }
        return;
    }

    if (strcmp(msg->tipo, MSG_FIN_JUEGO) == 0) {
        ej->fin_juego          = 1;
        ej->jugador.puntuacion = json_int(j, "puntuacion", ej->jugador.puntuacion);
        return;
    }

    if (strcmp(msg->tipo, MSG_RONDA_COMPLETA) == 0) {
        ej->ronda_completa           = 1;
        ej->ronda                    = json_int(j, "ronda", ej->ronda);
        ej->cantidad_extraterrestres = 0;
        ej->bala_activa              = 0;
        ej->cantidad_balas_alien     = 0;
        return;
    }

    if (strcmp(msg->tipo, "VELOCIDAD") == 0) {
        ej->velocidad_extraterrestre = json_float(j, "valor", ej->velocidad_extraterrestre);
        return;
    }

    if (strcmp(msg->tipo, "BUNKERS") == 0) {
        int pct = json_int(j, "porcentaje", 100);
        for (int i = 0; i < NUM_BUNKERS; i++)
            ej->bunkers[i].porcentaje_salud = pct;
        return;
    }
}

