package com.spaceinvaders.server;

import com.spaceinvaders.model.EstadoJuego;
import com.spaceinvaders.model.Extraterrestre;
import com.spaceinvaders.model.Jugador;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.time.LocalTime;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class ConsolaAdmin extends JFrame {

    private final ServidorJuego servidor;
    private static final DateTimeFormatter FMT = DateTimeFormatter.ofPattern("HH:mm:ss");

    /* Panel superior — estado global */
    private JLabel lblSesiones;
    private JLabel lblRonda;
    private JLabel lblVelocidad;
    private JLabel lblAliens;

    /* Panel central — tabla de jugadores */
    private DefaultTableModel modeloTabla;
    private JTable            tablaJugadores;
    private JTextArea         areaLog;

    /* Panel central — matriz de aliens (5 filas × 11 columnas) */
    private JButton[][] matrizBotones; // [fila][col]
    private int[][]     matrizIds;     // alien ID en cada celda (0 = sin asignar)

    /* Panel inferior — selector de sesión y campos de comandos */
    private JComboBox<Integer> cmbSesion;
    private JTextField     txtCrearFila;
    private JTextField     txtCrearCol;
    private JTextField     txtCrearPts;
    private JComboBox<String> cmbTipoAlien;
    private JComboBox<String> cmbOvniDir;
    private JTextField     txtOvniPts;
    private JTextField     txtVelocidad;
    private JTextField     txtBunkers;

    public ConsolaAdmin(ServidorJuego servidor) {
        super("spaCEinvaders — Consola Admin");
        this.servidor = servidor;
        matrizBotones = new JButton[5][11];
        matrizIds     = new int[5][11];

        setLayout(new BorderLayout(4, 4));
        add(panelEstado(),   BorderLayout.NORTH);
        add(panelCentral(),  BorderLayout.CENTER);
        add(panelComandos(), BorderLayout.SOUTH);
        setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
        setSize(1100, 760);
        setLocationRelativeTo(null);
        new Timer(500, e -> refrescar()).start();
        setVisible(true);
    }

    // ── Panel superior: estado global ────────────────────────────────────────
    private JPanel panelEstado() {
        JPanel p = new JPanel(new FlowLayout(FlowLayout.LEFT, 20, 6));
        p.setBackground(new Color(20, 20, 40));
        p.setBorder(BorderFactory.createTitledBorder(
                BorderFactory.createLineBorder(new Color(60, 120, 200)), "Estado del Servidor"));
        lblSesiones  = etiqueta("Sesiones: 0");
        lblRonda     = etiqueta("Ronda: —");
        lblVelocidad = etiqueta("Velocidad: —");
        lblAliens    = etiqueta("Aliens: —");
        p.add(lblSesiones);
        p.add(lblRonda);
        p.add(lblVelocidad);
        p.add(lblAliens);
        return p;
    }

    // ── Panel central: matriz aliens arriba + tabla/log abajo ────────────────
    private Component panelCentral() {
        JPanel contenedor = new JPanel(new BorderLayout(4, 4));
        contenedor.setBackground(new Color(10, 10, 20));

        // Matriz de aliens (parte superior, tamaño reducido)
        JPanel wrapMatriz = crearPanelMatriz();
        wrapMatriz.setPreferredSize(new Dimension(0, 105));
        contenedor.add(wrapMatriz, BorderLayout.NORTH);

        // Tabla de jugadores + log (parte inferior)
        modeloTabla = new DefaultTableModel(
                new String[]{"Sesión", "Vidas", "Puntuación", "Ronda", "Aliens"}, 0) {
            @Override public boolean isCellEditable(int r, int c) { return false; }
        };
        tablaJugadores = new JTable(modeloTabla);
        tablaJugadores.setBackground(new Color(15, 15, 30));
        tablaJugadores.setForeground(Color.LIGHT_GRAY);
        tablaJugadores.setGridColor(new Color(50, 50, 80));
        tablaJugadores.getTableHeader().setBackground(new Color(30, 30, 60));
        tablaJugadores.getTableHeader().setForeground(Color.WHITE);
        tablaJugadores.setFont(new Font("Monospaced", Font.PLAIN, 12));
        tablaJugadores.setRowHeight(20);
        tablaJugadores.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        JScrollPane scrollTabla = new JScrollPane(tablaJugadores);
        scrollTabla.setBorder(BorderFactory.createTitledBorder("Jugadores conectados"));

        areaLog = new JTextArea();
        areaLog.setEditable(false);
        areaLog.setBackground(new Color(10, 10, 20));
        areaLog.setForeground(new Color(150, 255, 150));
        areaLog.setFont(new Font("Monospaced", Font.PLAIN, 11));
        JScrollPane scrollLog = new JScrollPane(areaLog);
        scrollLog.setBorder(BorderFactory.createTitledBorder("Log de eventos"));

        JSplitPane split = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, scrollTabla, scrollLog);
        split.setDividerLocation(320);
        split.setResizeWeight(0.35);
        contenedor.add(split, BorderLayout.CENTER);

        return contenedor;
    }

    // ── Matriz de aliens: botón por celda, label=[fila][col], click=eliminar ─
    private JPanel crearPanelMatriz() {
        JPanel grid = new JPanel(new GridLayout(5, 11, 1, 1));
        grid.setBackground(new Color(10, 10, 20));

        for (int f = 0; f < 5; f++) {
            for (int c = 0; c < 11; c++) {
                JButton btn = new JButton("[" + f + "][" + c + "]");
                btn.setFont(new Font("Monospaced", Font.PLAIN, 8));
                btn.setBackground(new Color(25, 25, 40));
                btn.setForeground(new Color(100, 100, 120));
                btn.setMargin(new Insets(0, 0, 0, 0));
                btn.setEnabled(false);
                final int fila = f, col = c;
                btn.addActionListener(e -> eliminarAlienEnCelda(fila, col));
                matrizBotones[f][c] = btn;
                grid.add(btn);
            }
        }

        JPanel contenedor = new JPanel(new BorderLayout());
        contenedor.setBackground(new Color(10, 10, 20));
        contenedor.setBorder(BorderFactory.createTitledBorder(
                BorderFactory.createLineBorder(new Color(60, 120, 200)),
                "Matriz [fila][col] — verde=vivo (click=eliminar), rojo=muerto"));
        contenedor.add(grid, BorderLayout.CENTER);
        return contenedor;
    }

    // ── Panel inferior: selector de sesión + comandos admin ─────────────────
    private JPanel panelComandos() {
        JPanel p = new JPanel(new GridBagLayout());
        p.setBackground(new Color(20, 20, 40));
        p.setBorder(BorderFactory.createTitledBorder(
                BorderFactory.createLineBorder(new Color(60, 120, 200)), "Comandos Admin"));
        GridBagConstraints g = new GridBagConstraints();
        g.insets = new Insets(3, 5, 3, 5);
        g.anchor = GridBagConstraints.WEST;

        // Fila 0 — Selector de sesión
        g.gridy = 0; g.gridx = 0;
        JLabel lblSes = label("Sesión activa:");
        lblSes.setForeground(new Color(255, 220, 80));
        p.add(lblSes, g);
        g.gridx = 1; g.gridwidth = 3;
        cmbSesion = new JComboBox<>();
        cmbSesion.setBackground(new Color(35, 35, 55));
        cmbSesion.setForeground(Color.WHITE);
        cmbSesion.setPreferredSize(new Dimension(120, 24));
        p.add(cmbSesion, g);
        g.gridwidth = 1;
        g.gridx = 4;
        JLabel nota = new JLabel("← todos los comandos afectan esta sesión");
        nota.setForeground(new Color(150, 150, 180));
        nota.setFont(nota.getFont().deriveFont(10f));
        p.add(nota, g);

        // Fila 1 — Crear Alien (por posición de matriz [fila][col])
        g.gridy = 1; g.gridx = 0; p.add(label("Crear Alien:"), g);
        g.gridx = 1; p.add(new JLabel("Fila:"), g);
        g.gridx = 2; txtCrearFila = campo(3); p.add(txtCrearFila, g);
        g.gridx = 3; p.add(new JLabel("Col:"), g);
        g.gridx = 4; txtCrearCol = campo(3);  p.add(txtCrearCol,  g);
        g.gridx = 5; p.add(new JLabel("Pts:"), g);
        g.gridx = 6; txtCrearPts = campo(5);  p.add(txtCrearPts,  g);
        g.gridx = 7; p.add(new JLabel("Tipo:"), g);
        g.gridx = 8; cmbTipoAlien = new JComboBox<>(new String[]{"Calamar", "Cangrejo", "Pulpo"});
        p.add(cmbTipoAlien, g);
        g.gridx = 9;
        JButton btnCrear = boton("Crear");
        btnCrear.addActionListener(e -> crearAlien());
        p.add(btnCrear, g);

        // Fila 2 — OVNI
        g.gridy = 2; g.gridx = 0; p.add(label("OVNI:"), g);
        g.gridx = 1; p.add(new JLabel("Dir:"), g);
        g.gridx = 2; cmbOvniDir = new JComboBox<>(new String[]{"I-D", "D-I"}); p.add(cmbOvniDir, g);
        g.gridx = 3; p.add(new JLabel("Pts:"), g);
        g.gridx = 4; txtOvniPts = campo(5); p.add(txtOvniPts, g);
        g.gridx = 7;
        JButton btnOvni = boton("Lanzar OVNI");
        btnOvni.addActionListener(e -> lanzarOvni());
        p.add(btnOvni, g);

        // Fila 3 — Velocidad + Bunkers
        g.gridy = 3; g.gridx = 0; p.add(label("Velocidad:"), g);
        g.gridx = 1; g.gridwidth = 2; txtVelocidad = campo(6); p.add(txtVelocidad, g); g.gridwidth = 1;
        g.gridx = 3;
        JButton btnVel = boton("Aplicar");
        btnVel.addActionListener(e -> aplicarVelocidad());
        p.add(btnVel, g);
        g.gridx = 5; p.add(label("Bunkers %:"), g);
        g.gridx = 6; txtBunkers = campo(4); p.add(txtBunkers, g);
        g.gridx = 7;
        JButton btnBunkers = boton("Aplicar");
        btnBunkers.addActionListener(e -> aplicarBunkers());
        p.add(btnBunkers, g);

        // Fila 4 — Reducir / Aumentar vida
        g.gridy = 4; g.gridx = 0; p.add(label("Vidas jugador:"), g);
        g.gridx = 1; g.gridwidth = 2;
        p.add(new JLabel("(sesión seleccionada arriba)"), g);
        g.gridwidth = 1;
        g.gridx = 3;
        JButton btnReducir = boton("− Reducir vida");
        btnReducir.setBackground(new Color(140, 40, 40));
        btnReducir.addActionListener(e -> modificarVida(-1));
        p.add(btnReducir, g);
        g.gridx = 4;
        JButton btnAumentar = boton("+ Aumentar vida");
        btnAumentar.setBackground(new Color(40, 100, 40));
        btnAumentar.addActionListener(e -> modificarVida(+1));
        p.add(btnAumentar, g);

        return p;
    }

    // ── Acciones ─────────────────────────────────────────────────────────────
    private void crearAlien() {
        int idSesion = obtenerIdSesionCombo();
        if (idSesion < 0) { sinSesionWarning(); return; }
        try {
            int fila = Integer.parseInt(txtCrearFila.getText().trim());
            int col  = Integer.parseInt(txtCrearCol.getText().trim());
            int pts  = Integer.parseInt(txtCrearPts.getText().trim());
            String tipo = ((String) cmbTipoAlien.getSelectedItem()).toUpperCase();
            servidor.enviarComandoAdminASesion(idSesion,
                    "CrearEnMatriz " + fila + " " + col + " " + pts + " " + tipo);
        } catch (NumberFormatException ex) {
            JOptionPane.showMessageDialog(this, "Fila, Col y Pts deben ser enteros.", "Error", JOptionPane.ERROR_MESSAGE);
        }
    }

    private void lanzarOvni() {
        int idSesion = obtenerIdSesionCombo();
        if (idSesion < 0) { sinSesionWarning(); return; }
        try {
            String dir = (String) cmbOvniDir.getSelectedItem();
            int pts = Integer.parseInt(txtOvniPts.getText().trim());
            servidor.enviarComandoAdminASesion(idSesion, "OVNI " + dir + " " + pts);
        } catch (NumberFormatException ex) {
            JOptionPane.showMessageDialog(this, "Pts debe ser un entero.", "Error", JOptionPane.ERROR_MESSAGE);
        }
    }

    private void aplicarVelocidad() {
        int idSesion = obtenerIdSesionCombo();
        if (idSesion < 0) { sinSesionWarning(); return; }
        String v = txtVelocidad.getText().trim();
        if (v.isEmpty()) return;
        try {
            double vel = Double.parseDouble(v);
            if (vel <= 0) {
                JOptionPane.showMessageDialog(this, "La velocidad debe ser mayor a 0.", "Error", JOptionPane.ERROR_MESSAGE);
                return;
            }
            servidor.enviarComandoAdminASesion(idSesion, "Velocidad " + v);
        } catch (NumberFormatException ex) {
            JOptionPane.showMessageDialog(this, "Velocidad inválida.", "Error", JOptionPane.ERROR_MESSAGE);
        }
    }

    private void aplicarBunkers() {
        int idSesion = obtenerIdSesionCombo();
        if (idSesion < 0) { sinSesionWarning(); return; }
        String v = txtBunkers.getText().trim();
        if (v.isEmpty()) return;
        try {
            Integer.parseInt(v);
            servidor.enviarComandoAdminASesion(idSesion, "Bunkers " + v);
        } catch (NumberFormatException ex) {
            JOptionPane.showMessageDialog(this, "Porcentaje inválido.", "Error", JOptionPane.ERROR_MESSAGE);
        }
    }

    private void eliminarAlienEnCelda(int fila, int col) {
        int id = matrizIds[fila][col];
        if (id == 0) return;
        int idSesion = obtenerIdSesionCombo();
        if (idSesion < 0) { sinSesionWarning(); return; }
        servidor.enviarComandoAdminASesion(idSesion, "EliminarAlien " + id);
    }

    private void modificarVida(int delta) {
        int idSesion = obtenerIdSesionCombo();
        if (idSesion < 0) { sinSesionWarning(); return; }
        String cmd = (delta < 0) ? "ReducirVida " + idSesion : "AumentarVida " + idSesion;
        servidor.enviarComandoAdminASesion(idSesion, cmd);
    }

    private int obtenerIdSesionCombo() {
        Object sel = cmbSesion.getSelectedItem();
        if (sel == null) return -1;
        try { return (Integer) sel; } catch (Exception e) { return -1; }
    }

    private void sinSesionWarning() {
        JOptionPane.showMessageDialog(this,
                "No hay sesión seleccionada. Espera a que un jugador se conecte.",
                "Sin sesión", JOptionPane.WARNING_MESSAGE);
    }

    // ── Refresco periódico ───────────────────────────────────────────────────
    private void refrescar() {
        Map<Integer, MotorJuego> motores = servidor.getMotores();
        lblSesiones.setText("Sesiones: " + motores.size());
        modeloTabla.setRowCount(0);

        int    totalAliens = 0;
        double vel         = 1.0;
        int    ronda       = 1;

        try {
            // Actualizar combo de sesiones sin perder selección
            Integer sesionActual = (Integer) cmbSesion.getSelectedItem();
            List<Integer> idsNuevos = new ArrayList<>(motores.keySet());
            if (!idsNuevos.equals(comboIds())) {
                cmbSesion.removeAllItems();
                for (Integer id : idsNuevos) cmbSesion.addItem(id);
                if (sesionActual != null && idsNuevos.contains(sesionActual))
                    cmbSesion.setSelectedItem(sesionActual);
            }

            for (Map.Entry<Integer, MotorJuego> entry : motores.entrySet()) {
                MotorJuego  motor  = entry.getValue();
                EstadoJuego estado = motor.getEstado();
                int sesId = entry.getKey();
                Map<Integer, Jugador> jugadores = estado.obtenerJugadores();
                int aliensVivos = (int) estado.obtenerExtraterrestres()
                        .stream().filter(Extraterrestre::estaVivo).count();
                totalAliens += aliensVivos;
                vel   = motor.getVelocidadActual();
                ronda = estado.obtenerRonda();

                if (jugadores.isEmpty()) {
                    modeloTabla.addRow(new Object[]{sesId, 3, 0, ronda, aliensVivos});
                } else {
                    for (Jugador j : jugadores.values()) {
                        modeloTabla.addRow(new Object[]{
                                sesId, j.obtenerVidas(), j.obtenerPuntuacion(), ronda, aliensVivos});
                    }
                }
            }

            refrescarMatriz(motores);

        } catch (Exception ignored) { }

        if (!motores.isEmpty()) {
            lblRonda.setText("Ronda: " + ronda);
            lblVelocidad.setText(String.format("Velocidad: %.2f", vel));
            lblAliens.setText("Aliens: " + totalAliens);
        } else {
            lblRonda.setText("Ronda: —");
            lblVelocidad.setText("Velocidad: —");
            lblAliens.setText("Aliens: —");
            refrescarMatriz(motores); // motores está vacío → motor==null → reset interno
        }
    }

    private List<Integer> comboIds() {
        List<Integer> ids = new ArrayList<>();
        for (int i = 0; i < cmbSesion.getItemCount(); i++) ids.add(cmbSesion.getItemAt(i));
        return ids;
    }

    private void refrescarMatriz(Map<Integer, MotorJuego> motores) {
        int idSesion = obtenerIdSesionCombo();
        MotorJuego motor = (idSesion >= 0 && motores.containsKey(idSesion))
                ? motores.get(idSesion)
                : (motores.isEmpty() ? null : motores.values().iterator().next());

        if (motor == null) {
            for (int f = 0; f < 5; f++)
                for (int c = 0; c < 11; c++) {
                    matrizIds[f][c] = 0;
                    matrizBotones[f][c].setText("[" + f + "][" + c + "]");
                    matrizBotones[f][c].setBackground(new Color(25, 25, 40));
                    matrizBotones[f][c].setForeground(new Color(80, 80, 100));
                    matrizBotones[f][c].setEnabled(false);
                }
            return;
        }

        List<Extraterrestre> todos;
        synchronized (motor) {
            todos = new ArrayList<>(motor.getEstado().obtenerExtraterrestres());
        }

        for (int f = 0; f < 5; f++)
            for (int c = 0; c < 11; c++) {
                matrizIds[f][c] = 0;
                matrizBotones[f][c].setText("[" + f + "][" + c + "]");
                matrizBotones[f][c].setBackground(new Color(25, 25, 40));
                matrizBotones[f][c].setForeground(new Color(80, 80, 100));
                matrizBotones[f][c].setEnabled(false);
            }

        int baseId = 100;
        for (Extraterrestre e : todos) {
            int offset = e.obtenerId() - baseId;
            if (offset < 0 || offset >= 55) continue;
            int fila = offset / 11;
            int col  = offset % 11;
            matrizIds[fila][col] = e.obtenerId();
            JButton btn = matrizBotones[fila][col];
            boolean vivo = e.estaVivo();
            btn.setText("[" + fila + "][" + col + "]");
            btn.setBackground(vivo ? new Color(20, 70, 20) : new Color(60, 20, 20));
            btn.setForeground(vivo ? new Color(150, 255, 150) : new Color(180, 80, 80));
            btn.setEnabled(vivo);
        }
    }

    // ── Log público (llamado desde ServidorJuego) ────────────────────────────
    public void logEvento(String msg) {
        if (!SwingUtilities.isEventDispatchThread()) {
            SwingUtilities.invokeLater(() -> logEvento(msg));
            return;
        }
        areaLog.append("[" + LocalTime.now().format(FMT) + "] " + msg + "\n");
        areaLog.setCaretPosition(areaLog.getDocument().getLength());
    }

    // ── Helpers de UI ────────────────────────────────────────────────────────
    private JLabel crearLabel(String txt, Color color, Font font) {
        JLabel lbl = new JLabel(txt);
        lbl.setForeground(color);
        lbl.setFont(font);
        return lbl;
    }

    private JLabel etiqueta(String txt) {
        return crearLabel(txt, new Color(80, 200, 255), new Font("Monospaced", Font.BOLD, 13));
    }

    private JLabel label(String txt) {
        JLabel lbl = new JLabel(txt);
        return crearLabel(txt, Color.WHITE, lbl.getFont().deriveFont(Font.BOLD));
    }

    private JTextField campo(int cols) {
        JTextField f = new JTextField(cols);
        f.setBackground(new Color(35, 35, 55));
        f.setForeground(Color.WHITE);
        f.setCaretColor(Color.WHITE);
        return f;
    }

    private JButton boton(String txt) {
        JButton b = new JButton(txt);
        b.setBackground(new Color(40, 80, 160));
        b.setForeground(Color.WHITE);
        b.setFocusPainted(false);
        return b;
    }
}
