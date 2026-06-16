#ifndef CONSTANTES_H
#define CONSTANTES_H

/* Conexion */
#define IP_SERVIDOR        "127.0.0.1"
#define PUERTO_SERVIDOR    8080
#define PUERTO_ESPECTADOR  8081
#define TAMANO_BUFFER      4096

/* Ventana Raylib */
#define SCREEN_W         900
#define SCREEN_H         680

/* Coordenadas del mapa de juego */
#define ANCHO_JUEGO      100
#define ALTO_JUEGO       90

/* Jugador */
#define VIDAS_INICIALES  3
#define VELOCIDAD_CANON  5

/* Bala */
#define Y_JUGADOR_JUEGO  90

/* Bunkers */
#define NUM_BUNKERS      4

/* Extraterrestres */
#define MAX_EXTRATERR    55

/* Balas alienígenas */
#define MAX_BALAS_ALIEN  16

/* Tipos de extraterrestre */
#define TIPO_CALAMAR     0
#define TIPO_CANGREJO    1
#define TIPO_PULPO       2
#define TIPO_OVNI        3

/* Puntos por tipo */
#define PUNTOS_CALAMAR   10
#define PUNTOS_CANGREJO  20
#define PUNTOS_PULPO     40

/* Tipos de mensajes del protocolo */
#define MSG_CREAR_EXTRATERR  "CREAR_EXTRATERRESTRE"
#define MSG_CREAR_OVNI       "CREAR_OVNI"
#define MSG_VELOCIDAD        "VELOCIDAD"
#define MSG_BUNKERS          "BUNKERS"
#define MSG_ESTADO_JUEGO     "ESTADO_JUEGO"
#define MSG_EXTRATERR_ELIM   "EXTRATERRESTRE_ELIMINADO"
#define MSG_FIN_JUEGO        "FIN_JUEGO"
#define MSG_RONDA_COMPLETA   "RONDA_COMPLETA"

/* Hitbox genérica (layout igual a raylib Rectangle: x,y,width,height) */
typedef struct { float x, y, width, height; } HitBox;

#endif /* CONSTANTES_H */
