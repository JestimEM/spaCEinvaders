/*
 * prueba_cliente.c
 * Cliente C de prueba: conecta al servidor Java, envia JSON y recibe respuesta.
 * Compilar: gcc prueba_cliente.c -o prueba_cliente.exe -lws2_32
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define IP_SERVIDOR  "127.0.0.1"
#define PUERTO       8080
#define TAM_BUFFER   2048

/* Estructura simple de mensaje JSON */
typedef struct {
    char tipo[32];
    char carga[TAM_BUFFER];
} Mensaje;

/* Enviar linea terminada en \n */
static int enviar_json(SOCKET sock, const char *json) {
    char buf[TAM_BUFFER];
    snprintf(buf, sizeof(buf), "%s\n", json);
    int enviado = send(sock, buf, (int)strlen(buf), 0);
    return enviado > 0 ? 0 : -1;
}

/* Recibir linea del servidor */
static int recibir_linea(SOCKET sock, char *buffer, int tam) {
    int total = 0;
    char c;
    while (total < tam - 1) {
        int r = recv(sock, &c, 1, 0);
        if (r <= 0) break;
        if (c == '\n') break;
        if (c != '\r') buffer[total++] = c;
    }
    buffer[total] = '\0';
    return total;
}

/* Parser JSON minimalista: extrae valor de una clave */
static void extraer_campo(const char *json, const char *clave, char *valor, int tam) {
    char buscar[64];
    snprintf(buscar, sizeof(buscar), "\"%s\":", clave);
    const char *pos = strstr(json, buscar);
    if (!pos) { strncpy(valor, "no encontrado", tam); return; }
    pos += strlen(buscar);
    while (*pos == ' ') pos++;
    int i = 0;
    int entre_comillas = (*pos == '"');
    if (entre_comillas) pos++;
    while (*pos && i < tam - 1) {
        if (entre_comillas && *pos == '"') break;
        if (!entre_comillas && (*pos == ',' || *pos == '}')) break;
        valor[i++] = *pos++;
    }
    valor[i] = '\0';
}

int main(void) {
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in servidor;
    char buffer[TAM_BUFFER];

    printf("[CLIENTE] Inicializando Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "[CLIENTE] Error WSAStartup: %d\n", WSAGetLastError());
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "[CLIENTE] Error creando socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    servidor.sin_family      = AF_INET;
    servidor.sin_port        = htons(PUERTO);
    inet_pton(AF_INET, IP_SERVIDOR, &servidor.sin_addr);

    printf("[CLIENTE] Conectando a %s:%d...\n", IP_SERVIDOR, PUERTO);
    if (connect(sock, (struct sockaddr *)&servidor, sizeof(servidor)) == SOCKET_ERROR) {
        fprintf(stderr, "[CLIENTE] Error de conexion: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    printf("[CLIENTE] Conexion establecida.\n");

    /* Enviar JSON de prueba (mensaje CREAR_EXTRATERRESTRE) */
    const char *json_envio = "{\"tipo\":\"CREAR_EXTRATERRESTRE\",\"x\":5,\"y\":3,\"puntos\":20}";
    printf("[CLIENTE] Enviando: %s\n", json_envio);
    if (enviar_json(sock, json_envio) != 0) {
        fprintf(stderr, "[CLIENTE] Error al enviar.\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    /* Recibir respuesta del servidor */
    int recibidos = recibir_linea(sock, buffer, sizeof(buffer));
    if (recibidos > 0) {
        printf("[CLIENTE] JSON recibido: %s\n", buffer);

        /* Parsear campos del JSON de respuesta */
        char tipo[32], vidas[8], puntuacion[16], ronda[8];
        extraer_campo(buffer, "tipo",       tipo,       sizeof(tipo));
        extraer_campo(buffer, "vidas",      vidas,      sizeof(vidas));
        extraer_campo(buffer, "puntuacion", puntuacion, sizeof(puntuacion));
        extraer_campo(buffer, "ronda",      ronda,      sizeof(ronda));

        printf("[CLIENTE] Campos parseados:\n");
        printf("  tipo       = %s\n", tipo);
        printf("  vidas      = %s\n", vidas);
        printf("  puntuacion = %s\n", puntuacion);
        printf("  ronda      = %s\n", ronda);
        printf("[CLIENTE] Prueba de conexion y parser JSON: EXITOSA\n");
    } else {
        fprintf(stderr, "[CLIENTE] No se recibio respuesta del servidor.\n");
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
