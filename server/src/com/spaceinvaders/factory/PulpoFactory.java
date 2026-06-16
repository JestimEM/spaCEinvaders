package com.spaceinvaders.factory;

import com.spaceinvaders.model.Extraterrestre;
import com.spaceinvaders.model.Pulpo;

public class PulpoFactory implements ExtraterrestreFactory {
    @Override
    public Extraterrestre crearExtraterrestre(int id, int x, int y) {
        return new Pulpo(id, x, y);
    }

    @Override
    public Extraterrestre crearExtraterrestre(int id, int x, int y, int puntos) {
        return new Pulpo(id, x, y, puntos);
    }
}
