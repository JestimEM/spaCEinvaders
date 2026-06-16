package com.spaceinvaders.observer;

import com.spaceinvaders.model.EstadoJuego;

public interface GameObserver {
    void alCambiarEstadoJuego(EstadoJuego estado);
    void alEliminarExtraterrestre(int idExtraterrestre, int puntos);
    void alTerminarJuego(int puntuacionFinal);
    void alCompletarRonda(int nuevaRonda);
}
