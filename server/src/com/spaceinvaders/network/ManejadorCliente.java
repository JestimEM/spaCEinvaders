package com.spaceinvaders.network;

import com.spaceinvaders.model.EstadoJuego;
import com.spaceinvaders.observer.GameObserver;
import com.spaceinvaders.server.MotorJuego;
import com.spaceinvaders.server.ServidorJuego;

import java.io.*;
import java.net.Socket;

public class ManejadorCliente implements Runnable, GameObserver {
    protected final Socket socket;
    protected PrintWriter salida;
    protected BufferedReader entrada;
    protected int idCliente;
    private MotorJuego motor;
    protected ServidorJuego servidor;

    public ManejadorCliente(Socket socket, int idCliente) throws IOException {
        this.socket    = socket;
        this.idCliente = idCliente;
        socket.setTcpNoDelay(true);  // deshabilita Nagle — envío inmediato de cada JSON
        this.salida    = new PrintWriter(socket.getOutputStream(), true);
        this.entrada   = new BufferedReader(new InputStreamReader(socket.getInputStream()));
    }

    public void establecerMotor(MotorJuego motor)       { this.motor = motor; }
    public void establecerServidor(ServidorJuego srv)   { this.servidor = srv; }

    @Override
    public void run() {
        System.out.printf("[Red] Jugador %d conectado desde %s:%d%n",
                idCliente, socket.getInetAddress().getHostAddress(), socket.getPort());
        try {
            String linea;
            while ((linea = entrada.readLine()) != null) {
                manejarMensaje(linea);
            }
        } catch (IOException e) {
            System.err.printf("[Red] Jugador %d desconectado: %s%n", idCliente, e.getMessage());
        } finally {
            cerrar();
        }
    }

    protected void manejarMensaje(String mensaje) {
        if (motor != null) {
            motor.procesarMensajeCliente(idCliente, mensaje);
        }
    }

    public void enviarMensaje(String json) {
        if (salida != null) salida.println(json);
    }

    @Override
    public void alCambiarEstadoJuego(EstadoJuego estado) {
        if (motor == null) return;
        String json = AdaptadorMensaje.estadoJuegoAJson(
                estado, idCliente,
                motor.getBalaJugador(idCliente),
                motor.getBalasAlien(),
                motor.getVelocidadActual());
        enviarMensaje(json);
    }

    @Override
    public void alEliminarExtraterrestre(int id, int puntos) {
        String json = String.format("{\"tipo\":\"EXTRATERRESTRE_ELIMINADO\",\"id\":%d,\"puntos\":%d}", id, puntos);
        enviarMensaje(json);
    }

    @Override
    public void alTerminarJuego(int puntuacionFinal) {
        String json = String.format("{\"tipo\":\"FIN_JUEGO\",\"puntuacion\":%d}", puntuacionFinal);
        enviarMensaje(json);
    }

    @Override
    public void alCompletarRonda(int nuevaRonda) {
        String json = String.format("{\"tipo\":\"RONDA_COMPLETA\",\"ronda\":%d}", nuevaRonda);
        enviarMensaje(json);
    }

    public void cerrar() {
        try { socket.close(); } catch (IOException ignorado) {}
        if (servidor != null) servidor.desconectarJugador(this);
        System.out.printf("[Red] Jugador %d limpiado.%n", idCliente);
    }

    public int obtenerIdCliente() { return idCliente; }

    protected double getVelocidadMotor() {
        return motor != null ? motor.getVelocidadActual() : 1.0;
    }
}
