package com.spaceinvaders.observer;

import com.spaceinvaders.model.EstadoJuego;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

public class GameStateNotifier implements GameSubject {
    private final List<GameObserver> observadores = new CopyOnWriteArrayList<>();
    private EstadoJuego estadoJuego;

    public GameStateNotifier(EstadoJuego estadoJuego) {
        this.estadoJuego = estadoJuego;
    }

    @Override
    public void agregarObservador(GameObserver observador)    { observadores.add(observador); }

    @Override
    public void eliminarObservador(GameObserver observador)   { observadores.remove(observador); }

    @Override
    public void notificarCambioEstado() {
        for (GameObserver o : observadores) o.alCambiarEstadoJuego(estadoJuego);
    }

    @Override
    public void notificarExtraterrestreEliminado(int id, int puntos) {
        for (GameObserver o : observadores) o.alEliminarExtraterrestre(id, puntos);
    }

    @Override
    public void notificarFinJuego(int puntuacionFinal) {
        for (GameObserver o : observadores) o.alTerminarJuego(puntuacionFinal);
    }

    @Override
    public void notificarRondaCompleta(int nuevaRonda) {
        for (GameObserver o : observadores) o.alCompletarRonda(nuevaRonda);
    }

    public void establecerEstadoJuego(EstadoJuego estado) { this.estadoJuego = estado; }
}
