package com.spaceinvaders.model;

public class Pulpo extends Extraterrestre {
    public static final int PUNTOS_BASE = 40;

    public Pulpo(int id, int x, int y) {
        super(id, x, y, PUNTOS_BASE);
    }

    public Pulpo(int id, int x, int y, int puntos) {
        super(id, x, y, puntos);
    }

    @Override
    public String obtenerTipo() { return "PULPO"; }
}
