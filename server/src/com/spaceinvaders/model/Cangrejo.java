package com.spaceinvaders.model;

public class Cangrejo extends Extraterrestre {
    public static final int PUNTOS_BASE = 20;

    public Cangrejo(int id, int x, int y) {
        super(id, x, y, PUNTOS_BASE);
    }

    public Cangrejo(int id, int x, int y, int puntos) {
        super(id, x, y, puntos);
    }

    @Override
    public String obtenerTipo() { return "CANGREJO"; }
}
