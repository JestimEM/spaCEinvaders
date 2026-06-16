#ifndef RED_H
#define RED_H

#include "game_state.h"
#include "constants.h"

typedef struct {
    char tipo[32];
    char carga[TAMANO_BUFFER];
} Mensaje;

/* Prototipos */
int  red_conectar(const char *ip, int puerto);
void red_desconectar(int sockfd);         /* cierra socket (sin WSACleanup) */
void red_cerrar_socket(int sockfd);       /* shutdown + close (para detener hilo) */
void red_reset_buffer(void);              /* limpia ring buffer al reconectar */
int  red_enviar(int sockfd, const char *json);
int  red_recibir(int sockfd, char *buffer, int largo_maximo);
void red_parsear_mensaje(const char *json, Mensaje *msg);
void red_aplicar_mensaje(const Mensaje *msg, EstadoJuego *ej);

#endif /* RED_H */
