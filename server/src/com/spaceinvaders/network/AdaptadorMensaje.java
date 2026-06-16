package com.spaceinvaders.network;

import com.spaceinvaders.model.*;

import java.util.List;

public class AdaptadorMensaje {

    /* 0=CALAMAR 1=CANGREJO 2=PULPO — debe coincidir con constants.h del cliente C */
    private static int tipoAInt(String tipo) {
        if ("CALAMAR".equals(tipo))  return 0;
        if ("CANGREJO".equals(tipo)) return 1;
        return 2;
    }

    /* ================================================================
       Bloque compartido: serializa todos los campos que tanto el jugador
       como el espectador necesitan en el mismo formato.
       sb queda abierto (sin cierre "}") para que el llamador pueda
       añadir campos extra antes de cerrar.
       ================================================================ */
    private static void appendCamposComunes(StringBuilder sb,
                                            int vidas, int puntuacion,
                                            double velocidad, int ronda, int canonX,
                                            BalaJugador bala,
                                            EstadoJuego estado,
                                            List<BalaAlien> balasAlien) {
        sb.append("{\"tipo\":\"ESTADO_JUEGO\"");
        sb.append(",\"vidas\":").append(vidas);
        sb.append(",\"puntuacion\":").append(puntuacion);
        sb.append(",\"velocidad\":").append(velocidad);
        sb.append(",\"ronda\":").append(ronda);
        sb.append(",\"canonX\":").append(canonX);

        /* Bala del jugador (server-side) */
        boolean balaActiva = bala != null && bala.isActiva();
        sb.append(",\"balaActiva\":").append(balaActiva);
        if (balaActiva) {
            sb.append(",\"balaX\":").append(bala.getX());
            sb.append(",\"balaY\":").append(bala.getY());
        }

        /* Aliens vivos — array plano: [id,x,y,tipo_int,pts, ...] */
        sb.append(",\"aliens\":[");
        boolean primero = true;
        for (Extraterrestre e : estado.obtenerExtraterrestres()) {
            if (!e.estaVivo()) continue;
            if (!primero) sb.append(",");
            sb.append(e.obtenerId()).append(",")
              .append(e.obtenerX()).append(",")
              .append(e.obtenerY()).append(",")
              .append(tipoAInt(e.obtenerTipo())).append(",")
              .append(e.obtenerPuntos());
            primero = false;
        }
        sb.append("]");

        /* Bunkers */
        sb.append(",\"bunkers\":[");
        Bunker[] bunkers = estado.obtenerBunkers();
        for (int i = 0; i < bunkers.length; i++) {
            if (i > 0) sb.append(",");
            sb.append(bunkers[i].obtenerPorcentajeSalud());
        }
        sb.append("]");

        /* OVNI — array plano: [activo(0/1), x, pts] */
        Ovni ovni = estado.obtenerOvni();
        sb.append(",\"ovni\":[").append(ovni.estaActivo() ? 1 : 0)
          .append(",").append(ovni.estaActivo() ? ovni.obtenerX() : 0)
          .append(",").append(ovni.estaActivo() ? ovni.obtenerPuntos() : 0)
          .append("]");

        /* Balas alienígenas — array plano: [x,y, x,y, ...] */
        sb.append(",\"balasAlien\":[");
        if (balasAlien != null) {
            boolean primera = true;
            for (BalaAlien ba : balasAlien) {
                if (!ba.isActiva()) continue;
                if (!primera) sb.append(",");
                sb.append(ba.getX()).append(",").append(ba.getY());
                primera = false;
            }
        }
        sb.append("]");
        /* sb queda sin "}" — el llamador cierra */
    }

    /* ================================================================
       Vista jugador: datos del jugador identificado por idCliente.
       ================================================================ */
    public static String estadoJuegoAJson(EstadoJuego estado,
                                          int idCliente,
                                          BalaJugador bala,
                                          List<BalaAlien> balasAlien,
                                          double velocidad) {
        Jugador j = estado.obtenerJugador(idCliente);
        StringBuilder sb = new StringBuilder();
        appendCamposComunes(sb,
                j.obtenerVidas(), j.obtenerPuntuacion(),
                velocidad, estado.obtenerRonda(), j.obtenerPosicionCanon(),
                bala, estado, balasAlien);
        sb.append("}");
        return sb.toString();
    }

    /* ================================================================
       Vista espectador: mismos datos + campo "jugadores" con todos los
       cañones activos (útil para partidas multi-jugador futuras).
       ================================================================ */
    public static String estadoJuegoAJson(EstadoJuego estado,
                                          BalaJugador bala,
                                          List<BalaAlien> balasAlien,
                                          double velocidad) {
        Jugador primer = estado.obtenerJugador();
        StringBuilder sb = new StringBuilder();
        appendCamposComunes(sb,
                primer.obtenerVidas(), primer.obtenerPuntuacion(),
                velocidad, estado.obtenerRonda(), primer.obtenerPosicionCanon(),
                bala, estado, balasAlien);

        /* Campo extra: posición de todos los cañones para el espectador */
        sb.append(",\"jugadores\":[");
        boolean primJ = true;
        for (java.util.Map.Entry<Integer, Jugador> e : estado.obtenerJugadores().entrySet()) {
            if (!primJ) sb.append(",");
            sb.append("{\"id\":").append(e.getKey())
              .append(",\"canonX\":").append(e.getValue().obtenerPosicionCanon())
              .append(",\"vidas\":").append(e.getValue().obtenerVidas())
              .append("}");
            primJ = false;
        }
        sb.append("]");

        sb.append("}");
        return sb.toString();
    }

    /* Sobrecargas de compatibilidad */
    public static String estadoJuegoAJson(EstadoJuego estado, double velocidad) {
        return estadoJuegoAJson(estado, (BalaJugador) null, null, velocidad);
    }

    public static String estadoJuegoAJson(EstadoJuego estado,
                                          List<BalaAlien> balasAlien,
                                          double velocidad) {
        return estadoJuegoAJson(estado, (BalaJugador) null, balasAlien, velocidad);
    }

    /* ================================================================
       Mensajes de control
       ================================================================ */
    public static String construirMsgCrearExtraterrestre(int x, int y, int puntos) {
        return String.format("{\"tipo\":\"CREAR_EXTRATERRESTRE\",\"x\":%d,\"y\":%d,\"puntos\":%d}", x, y, puntos);
    }

    public static String construirMsgCrearOvni(String direccion, int puntos) {
        return String.format("{\"tipo\":\"CREAR_OVNI\",\"direccion\":\"%s\",\"puntos\":%d}", direccion, puntos);
    }

    public static String construirMsgVelocidad(double velocidad) {
        return String.format("{\"tipo\":\"VELOCIDAD\",\"valor\":%.1f}", velocidad);
    }

    public static String construirMsgBunkers(int porcentaje) {
        return String.format("{\"tipo\":\"BUNKERS\",\"porcentaje\":%d}", porcentaje);
    }
}
