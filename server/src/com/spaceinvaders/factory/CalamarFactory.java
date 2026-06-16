package com.spaceinvaders.factory;

import com.spaceinvaders.model.Extraterrestre;
import com.spaceinvaders.model.Calamar;

public class CalamarFactory implements ExtraterrestreFactory {
    @Override
    public Extraterrestre crearExtraterrestre(int id, int x, int y) {
        return new Calamar(id, x, y);
    }

    @Override
    public Extraterrestre crearExtraterrestre(int id, int x, int y, int puntos) {
        return new Calamar(id, x, y, puntos);
    }
}
