package com.spaceinvaders.observer;

public interface GameSubject {
    void agregarObservador(GameObserver observador);
    void eliminarObservador(GameObserver observador);
    void notificarCambioEstado();
    void notificarExtraterrestreEliminado(int id, int puntos);
    void notificarFinJuego(int puntuacionFinal);
    void notificarRondaCompleta(int nuevaRonda);
}
