#include "../include/game_state.h"
#include <string.h>

static const int BUNKER_X[NUM_BUNKERS] = {12, 37, 62, 87};
static const int BUNKER_Y = 70;

void estado_juego_inicializar(EstadoJuego *ej) {
    memset(ej, 0, sizeof(EstadoJuego));
    jugador_inicializar(&ej->jugador);
    ovni_inicializar(&ej->ovni);
    for (int i = 0; i < NUM_BUNKERS; i++) {
        bunker_inicializar(&ej->bunkers[i], i, BUNKER_X[i], BUNKER_Y);
    }
    ej->cantidad_extraterrestres  = 0;
    ej->velocidad_extraterrestre  = 1.0f;
    ej->ronda                     = 1;
    ej->fin_juego                 = 0;
    ej->ronda_completa            = 0;
    ej->bala_activa               = 0;
    ej->bala_x                    = 0;
    ej->bala_y                    = 0;
    ej->cantidad_balas_alien      = 0;
}

void estado_juego_reiniciar_ronda(EstadoJuego *ej) {
    ej->cantidad_extraterrestres = 0;
    memset(ej->extraterrestres, 0, sizeof(ej->extraterrestres));
    ovni_desactivar(&ej->ovni);
    ej->ronda_completa       = 0;
    ej->bala_activa          = 0;
    ej->cantidad_balas_alien = 0;
    memset(ej->balas_alien, 0, sizeof(ej->balas_alien));
}
