package com.spaceinvaders.model;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class EstadoJuego {
    private List<Extraterrestre> extraterrestres;
    private Map<Integer, Jugador> jugadores;
    private Ovni ovni;
    private Bunker[] bunkers;
    private int ronda;

    public EstadoJuego() {
        this.extraterrestres = new ArrayList<>();
        this.jugadores       = new HashMap<>();
        this.ovni            = new Ovni();
        this.bunkers         = new Bunker[]{ new Bunker(0), new Bunker(1), new Bunker(2), new Bunker(3) };
        this.ronda           = 1;
    }

    /* ---------- Aliens ---------- */
    public void agregarExtraterrestre(Extraterrestre e) { extraterrestres.add(e); }
    public void eliminarExtraterrestre(int id)          { extraterrestres.removeIf(e -> e.obtenerId() == id); }
    public boolean todosEliminados()                    { return extraterrestres.stream().noneMatch(Extraterrestre::estaVivo); }
    public List<Extraterrestre> obtenerExtraterrestres(){ return extraterrestres; }

    public void siguienteRonda() {
        ronda++;
    }

    /* ---------- Jugadores (multi-player) ---------- */
    public Jugador obtenerJugador(int idCliente) {
        return jugadores.computeIfAbsent(idCliente, k -> new Jugador());
    }

    /* Backward-compat: retorna el jugador 1, o el primero disponible */
    public Jugador obtenerJugador() {
        if (jugadores.isEmpty()) return obtenerJugador(1);
        return jugadores.values().iterator().next();
    }

    public Map<Integer, Jugador> obtenerJugadores() {
        return Collections.unmodifiableMap(jugadores);
    }

    public void reiniciarJugador(int idCliente) {
        Jugador j = jugadores.get(idCliente);
        if (j != null) j.reiniciar();
    }

    public void eliminarJugador(int idCliente) {
        jugadores.remove(idCliente);
    }

    /* ---------- Estado compartido ---------- */
    public Ovni     obtenerOvni()     { return ovni; }
    public Bunker[] obtenerBunkers()  { return bunkers; }
    public int      obtenerRonda()    { return ronda; }
}
