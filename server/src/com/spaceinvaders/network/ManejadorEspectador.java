package com.spaceinvaders.network;

import com.spaceinvaders.model.BalaAlien;
import com.spaceinvaders.model.BalaJugador;
import com.spaceinvaders.model.EstadoJuego;
import com.spaceinvaders.server.MotorJuego;

import java.io.IOException;
import java.net.Socket;
import java.util.List;
import java.util.Map;

public class ManejadorEspectador extends ManejadorCliente {

    /* Motor de la sesión que este espectador eligió observar */
    private MotorJuego motorEspectado = null;

    public ManejadorEspectador(Socket socket, int idCliente) throws IOException {
        super(socket, idCliente);
    }

    public void setMotorEspectado(MotorJuego m) { motorEspectado = m; }

    /* ----------------------------------------------------------------
       Ciclo de vida del espectador:
       1. Enviar lista de sesiones activas.
       2. Esperar que el cliente elija una.
       3. Suscribirse solo a ese notificador + guardar referencia al motor.
       4. Leer mensajes del cliente (ignorados).
       ---------------------------------------------------------------- */
    @Override
    public void run() {
        System.out.printf("[Red] Espectador %d conectado desde %s:%d%n",
                idCliente,
                socket.getInetAddress().getHostAddress(),
                socket.getPort());
        try {
            enviarSesiones();
            int idSesion = esperarSeleccion();
            if (idSesion >= 0 && servidor != null) {
                servidor.suscribirEspectador(this, idSesion);
            }
            String linea;
            while ((linea = entrada.readLine()) != null) {
                manejarMensaje(linea);
            }
        } catch (IOException e) {
            System.err.printf("[Red] Espectador %d desconectado: %s%n",
                    idCliente, e.getMessage());
        } finally {
            cerrar();
        }
    }

    @Override
    protected void manejarMensaje(String mensaje) { /* solo lectura */ }

    /* La velocidad viene del motor de la sesión observada */
    @Override
    protected double getVelocidadMotor() {
        return motorEspectado != null ? motorEspectado.getVelocidadActual() : 1.0;
    }

    @Override
    public void alCambiarEstadoJuego(EstadoJuego estado) {
        BalaJugador balaJug = null;
        List<BalaAlien> balasAlien = null;
        if (motorEspectado != null) {
            balasAlien = motorEspectado.getBalasAlien();
            /* Bala del primer jugador de la sesión */
            if (!estado.obtenerJugadores().isEmpty()) {
                int primerId = estado.obtenerJugadores().keySet().iterator().next();
                balaJug = motorEspectado.getBalaJugador(primerId);
            }
        }
        enviarMensaje(AdaptadorMensaje.estadoJuegoAJson(estado, balaJug, balasAlien, getVelocidadMotor()));
    }

    /* ----------------------------------------------------------------
       Construye y envía {"tipo":"SESIONES","ids":[1,2,...]}
       ---------------------------------------------------------------- */
    private void enviarSesiones() {
        if (servidor == null) { enviarMensaje("{\"tipo\":\"SESIONES\",\"ids\":[]}"); return; }
        Map<Integer, MotorJuego> motores = servidor.getMotores();
        StringBuilder sb = new StringBuilder("{\"tipo\":\"SESIONES\",\"ids\":[");
        boolean primero = true;
        for (Integer id : motores.keySet()) {
            if (!primero) sb.append(",");
            sb.append(id);
            primero = false;
        }
        sb.append("]}");
        enviarMensaje(sb.toString());
    }

    /* ----------------------------------------------------------------
       Lee líneas hasta recibir {"tipo":"SELECCIONAR_SESION","id":N}
       Timeout de 30 segundos.
       ---------------------------------------------------------------- */
    private int esperarSeleccion() throws IOException {
        long deadline = System.currentTimeMillis() + 30_000;
        while (System.currentTimeMillis() < deadline) {
            String linea = entrada.readLine();
            if (linea == null) return -1;
            if (linea.contains("SELECCIONAR_SESION")) {
                int idx = linea.indexOf("\"id\":");
                if (idx >= 0) {
                    String rest = linea.substring(idx + 5).trim();
                    try {
                        return Integer.parseInt(rest.replaceAll("[^0-9].*", ""));
                    } catch (NumberFormatException ignored) { }
                }
            }
        }
        return -1;
    }
}
