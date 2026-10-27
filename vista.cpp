#include "vista.h"
#include "ui_vista.h"

#include <QMessageBox>
#include <QDate>
#include <QTime>
#include <QTimer>
#include <QInputDialog>
#include <QFileInfo>
#include <QFileDialog>
#include <QListWidgetItem>

Vista::Vista(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Vista){

    ui->setupUi(this);
    initUi();

}

Vista::~Vista(){
    delete ui;
}

//======================================================================================
// Funciones Init
//======================================================================================
void Vista::initView(View* view){
    m_view = view;
}

void Vista::initLienzo(Lienzo* lienzo){
    m_lienzo = lienzo;
    // 🎯 EL ANCLAJE CRÍTICO: Conectamos la señal del cerebro directamente al repintado del Lienzo
    if (m_view && m_lienzo) {
        connect(m_view, &View::actualizarLienzo, m_lienzo, &Lienzo::renderLienzo);
    }
}

void Vista::initUi(){
    initGrupoElementos();
    initBarraEstado();
    initAnimacion();
    setEstilos();
}

void Vista::initGrupoElementos(){
    m_grupoIzdo = new QButtonGroup(this);
    m_grupoIzdo->setExclusive(true);
    m_grupoIzdo->addButton(ui->btnSeleccion);
    m_grupoIzdo->addButton(ui->btnModoEnlace);
    m_grupoIzdo->addButton(ui->btnModoRotar);
    m_grupoIzdo->addButton(ui->btnCarbono);
    m_grupoIzdo->addButton(ui->btnHidrogeno);
    m_grupoIzdo->addButton(ui->btnOxigeno);
    m_grupoIzdo->addButton(ui->btnNitrogeno);
    m_grupoIzdo->addButton(ui->btnAzufre);
    m_grupoIzdo->addButton(ui->btnTablaPeriodica);

}

void Vista::initBarraEstado(){

    m_lblMesaje = new QLabel();
    m_fecha     = new QLabel();
    m_hora      = new QLabel();

    QList<QLabel*> etiquetas = {m_lblMesaje, m_fecha, m_hora};
    for(QLabel* label: etiquetas){
        label->setFrameShape(QFrame::WinPanel);
        label->setFrameShadow(QFrame::Sunken);
        label->setAlignment(Qt::AlignCenter);
    }
    m_lblMesaje->setAlignment(Qt::AlignLeft);

    ui->barraEstado->addWidget(m_lblMesaje, 1);
    ui->barraEstado->addPermanentWidget(m_fecha);
    ui->barraEstado->addPermanentWidget(m_hora);

    m_fecha->setText(QDate::currentDate().toString("dddd, d 'de' MMMM 'de' yyyy"));
    m_hora->setText(QTime::currentTime().toString("hh:mm:ss"));

    // Hace que se refresque la hora y fecha cada segundo
    QTimer* disparador = new QTimer(this);
    connect(disparador, &QTimer::timeout, this, [this](){
        m_fecha->setText(QDate::currentDate().toString("dddd, d 'de' MMMM 'de' yyyy"));
        m_hora->setText(QTime::currentTime().toString("hh:mm:ss"));
    });
    disparador->start(1000);
}

void Vista::initAnimacion(){
    m_timerAnimacion = new QTimer();
    m_timerAnimacion->setInterval(200);

    connect(m_timerAnimacion, &QTimer::timeout, this, &Vista::on_btnPasoAdelante_clicked);


    // 🎯 REPARACIÓN CRÍTICA: Asignamos memoria real al temporizador para evitar el cuelgue
    m_timerVibracion = new QTimer();
    m_timerVibracion->setInterval(30);
    connect(m_timerVibracion, &QTimer::timeout, this, &Vista::actualizarFrameVibracion);
}

//======================================================================================
// Funciones SLOT Publicos
//======================================================================================
void Vista::actualizarUi(Estado estado){

    // 1. Capa de Metadatos de Aplicación y Título de Ventana
    actualizarTituloYMetadatos(estado);

    // 2. Capa de Permisos de Menús de Barra Superior y Acciones de Archivo
    actualizarAccionesYMenus(estado);

    // 3. Capa de Sincronización Óptica de la Barra de Herramientas Izquierda (Átomos/Modos)
    actualizarControlesEdicion(estado);

    // 4. Capa de Intercambio Dinámico Superior Derecho (Formulario vs Lista de Frecuencias)
    actualizarPanelResultados(estado);

    // 5. Capa Espectroscópica e Inferior (Curvas, Perfiles y Automatización de Arranque IR)
    actualizarGraficaEInferior(estado);
}

void Vista::setMenuBotonDerecho(int idAtomo, int idEnlace){
    QMenu menuDerecho(this);

    // CASO A: El clic derecho cayó sobre un átomo válido
    if (idAtomo >= 0) {
        QAction* actEliminarAtomo = menuDerecho.addAction(tr("Eliminar Átomo"));
        QAction* accionSeleccionada = menuDerecho.exec(QCursor::pos());

        if (accionSeleccionada == actEliminarAtomo) {
            m_view->eliminarAtomo(idAtomo); // El cerebro limpia la memoria
        }
        return; // ¡CRUCIAL! Salimos inmediatamente del método para evitar lecturas de basura
    }

    // CASO B: El clic derecho cayó sobre un enlace limpio
    if (idEnlace >= 0) {
        QAction* actEliminarEnlace = menuDerecho.addAction(tr("Eliminar Enlace"));
        QAction* accionSeleccionada = menuDerecho.exec(QCursor::pos());

        if (accionSeleccionada == actEliminarEnlace) {
            m_view->eliminarEnlace(idEnlace);
        }
        return;
    }
}

//======================================================================================
// Funciones Privadas
//======================================================================================

void Vista::setEstilos(){
    QString qss =
        "QMainWindow {"
        "    background-color: #0F172A;"
        "    color: #E2E8F0;"
        "}"
        "QMenuBar {"
        "    background-color: #1E293B;"
        "    color: #E2E8F0;"
        "    border-bottom: 1px solid #334155;"
        "}"
        "QMenuBar::item:selected {"
        "    background-color: #38BDF8;"
        "    color: #0F172A;"
        "}"
        "QMenu {"
        "    background-color: #1E293B;"
        "    color: #E2E8F0;"
        "    border: 1px solid #334155;"
        "}"
        "QMenu::item:selected {"
        "    background-color: #38BDF8;"
        "    color: #0F172A;"
        "}"
        "QToolBar#barraSuperior {"
        "    background-color: #1E293B;"
        "    border-bottom: 1px solid #334155;"
        "    spacing: 4px;"
        "    padding: 4px;"
        "}"
        "QGraphicsView#visorMolecular {"
        "    background-color: #020617;"
        "    border: 2px solid #1E293B;"
        "    border-radius: 6px;"
        "}"
        "QGraphicsView#visorMolecular:disabled {"
        "    background-color: #1E293B;" /* Fondo grisáceo oscuro similar a las barras */
        "    border: 2px solid #334155;" /* Borde atenuado */
        "}"
        "QFrame#barraIzda {"
        "    background-color: #1E293B;"
        "    border-right: 1px solid #334155;"
        "}"
        "QFrame#barraIzda QPushButton {"
        "    background-color: #334155;"
        "    border: 1px solid #475569;"
        "    border-radius: 8px;"
        "    padding: 6px;"
        "    margin: 2px 6px;"
        "}"
        "QFrame#barraIzda QPushButton:hover {"
        "    background-color: #475569;"
        "    border-color: #38BDF8;"
        "}"
        "QFrame#barraIzda QPushButton:checked {"
        "    background-color: #0EA5E9;"
        "    border: 2px solid #38BDF8;"
        "}"
        "QPushButton#btnCarbono:checked   { background-color: #475569; border-color: #94A3B8; }"
        "QPushButton#btnHidrogeno:checked { background-color: #F8FAFC; border-color: #38BDF8; color: #0F172A; }"
        "QPushButton#btnOxigeno:checked   { background-color: #EF4444; border-color: #F87171; }"
        "QPushButton#btnNitrogeno:checked { background-color: #3B82F6; border-color: #60A5FA; }"
        "QPushButton#btnAzufre:checked    { background-color: #EAB308; border-color: #FACC15; }"
        "QFrame#barraDerecha {"
        "    background-color: #1E293B;"
        "    border-left: 1px solid #334155;"
        "}"
        "QFrame#barraDerecha QWidget:disabled {"
        "   background-color: #2e2e2e;"  // Fondo gris oscuro al desactivar
        "   color: #777777;"              // Texto gris apagado
        "   border: 1px solid #444444;"   // Borde suave
        "}"
        "QFrame#frameSoftware, QFrame#frameEjecucion, QFrame#frameResultados, QFrame#frameFrecuencias {"
        "    background-color: #111827;"
        "    border: 1px solid #374151;"
        "    border-radius: 6px;"
        "    padding: 4px;"
        "}"
        "QComboBox {"
        "    background-color: #1F2937;"
        "    color: #F3F4F6;"
        "    border: 1px solid #4B5563;"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #1F2937;" // Fondo del menú desplegable
        "    color: #F3F4F6;"            // Color del texto de las opciones
        "    selection-background-color: #38BDF8;" // Color de fondo al pasar el ratón
        "    selection-color: #0F172A;"            // Color del texto al pasar el ratón
        "    border: 1px solid #4B5563;"
        "}"
        // 🎯 ESTILO PERSONALIZADO PARA EL VISOR DE ARGUMENTOS QM
        "QLineEdit#etArgumentos {"
        "    background-color: #030712;" /* Fondo negro de terminal */
        "    color: #38BDF8;"            /* Texto azul cian brillante para resaltar comandos */
        "    font-family: 'Consolas', 'Courier New', monospace;" /* Fuente de código */
        "    font-size: 13px;"
        "    font-weight: bold;"
        "    border: 1px solid #374151;" /* Borde gris oscuro a juego con los frames */
        "    border-radius: 4px;"
        "    padding: 5px 8px;"
        "}"

        /* Comportamiento visual adaptativo cuando la barra lateral se bloquea */
        "QLineEdit#etArgumentos:disabled {"
        "    background-color: #1F2937;" /* Fondo gris oscuro atenuado */
        "    color: #4B5563;"            /* Texto apagado */
        "    border-color: #374151;"
        "}"
        "QPushButton#btnLanzar {"
        "    background-color: #10B981;"
        "    color: white;"
        "    font-weight: bold;"
        "    border-radius: 4px;"
        "    padding: 6px 12px;"
        "}"
        "QPushButton#btnLanzar:hover { background-color: #059669; }"
        "QPushButton#btnAbortar {"
        "    background-color: #EF4444;"
        "    color: white;"
        "    font-weight: bold;"
        "    border-radius: 4px;"
        "    padding: 6px 12px;"
        "}"
        "QPushButton#btnAbortar:hover { background-color: #DC2626; }"
        "QListWidget#listaFrecuencias {"
        "    background-color: #030712;"
        "    color: #10B981;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    border: 1px solid #1F2937;"
        "    border-radius: 4px;"
        "}"
        "QFrame#barraDerecha QLabel {"
        "    color: #94A3B8;"
        "    font-weight: bold;"
        "    font-size: 15px;" // ◄ Incrementado para las etiquetas fijas (Energía, Dipolo, etc.)
        "}"

        // Selector específico para los valores mutables que cambian de color verde terminal:
        "QLabel#lblEnergia, QLabel#lblCalorFormacion, "
        "QLabel#lblIonizacion, QLabel#lblDipolo, QLabel#lblTiempoCpu {"
        "    color: #10B981;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    font-size: 16px;" // ◄ Incrementado para los datos variables cuánticos
        "    font-weight: bold;"
        "}"

        "QListWidget#listaFrecuencias {"
        "    background-color: #030712;"
        "    color: #10B981;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    font-size: 14px;" // ◄ Escalado sutil para la lista de modos IR verdes
        "    border: 1px solid #1F2937;"
        "    border-radius: 4px;"
        "}";



    this->setStyleSheet(qss);
}

void Vista::stopAnimaciones(const Estado& estado) {
    // 1. Detener y limpiar la animación de Frecuencias (Vibración)
    if (m_timerVibracion && m_timerVibracion->isActive()) {
        m_timerVibracion->stop();
    }
    m_idxModoActivo = -1;
    ui->wGrafica->setModoActivo(-1);

    // 🎯 REPARACIÓN: Si el modo de edición NO es rotación, forzamos el reposo de la molécula
    if (m_view) {
        if (estado.modoEditor != ModoEditor::ModoRotacion3D) {
            m_view->actualizarFaseVibracion(-1, 0.0f, 0.0f); // Limpia la deformación en la RAM
        } else if (m_timerVibracion && !m_timerVibracion->isActive() && estado.hayFrecuencias) {
            // Si es rotación pero el timer se paró por error, dejamos que conserve el reposo
            m_view->actualizarFaseVibracion(-1, 0.0f, 0.0f);
        }
    }

    // 2. Detener y limpiar la animación de Trayectorias (Película)
    if (!estado.hayTrayectoria) {
        if (m_timerAnimacion && m_timerAnimacion->isActive()) {
            m_timerAnimacion->stop();
        }
        ui->btnPlayAnimacion->setText(tr("▶️ Play"));
        ui->frameTrayectoria->setVisible(false);
        ui->sliderPasos->setRange(0, 0);
        ui->sliderPasos->setValue(0);
    }

    // 3. SINCRO DE LA BARRA DE ESTADO... (dejar como estaba)
    if (!estado.estaEmpezado) {
        m_lblMesaje->setText(tr(" Listo. Cree un nuevo proyecto o abra un archivo para comenzar."));
    } else {
        QString txt = "";
        switch (estado.modoEditor) {
        case ModoSeleccion:   txt = tr(" Modo Selección | Use el puntero para interactuar con los átomos."); break;
        case ModoCrearEnlace: txt = tr(" Modo Enlace 🔗 | Seleccione dos átomos para enlazarlos."); break;
        case ModoRotacion3D:  txt = tr(" Modo Rotación 3D 🔄 | Arrastre el lienzo para orbitar la molécula."); break;
        case ModoDibujo:      txt = tr(" Modo Dibujo 🧪 | Añadiendo elemento: %1 (Clic en lienzo para colocar)").arg(m_view->getAtomoActivo()); break;
        }
        m_lblMesaje->setText(txt);
    }
}

bool Vista::sePuedeCerrar(){
    if (!m_view) return true;

    Estado estado = m_view->getEstado();

    // Si no hay cambios sin guardar o está vacío, es seguro continuar
    if (estado.estaGuardado || estado.estaVacio) {
        return true;
    }

    // Cuadro de diálogo de advertencia
    QMessageBox::StandardButton respuesta = QMessageBox::warning(
        this,
        tr("Cambios sin guardar"),
        tr("El proyecto actual tiene cambios sin guardar.\n¿Desea guardarlos antes de continuar?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );

    if (respuesta == QMessageBox::Save) {
        // Si no tiene ruta, la Vista debe pedirla antes de proceder
        if (m_view->getRutaActual().isEmpty()) {
            QString ruta = QFileDialog::getSaveFileName(
                this,
                tr("Guardar Proyecto Como"),
                "",
                tr("Archivos de VisMol (*.vismol)")
                );

            if (ruta.isEmpty()) {
                return false; // ◄ ¡SOLUCIÓN!: El usuario canceló el guardado, detenemos el proceso de abrir
            }

            if (!ruta.endsWith(".vismol", Qt::CaseInsensitive)) {
                ruta += ".vismol";
            }
            m_view->setRutaActual(ruta);
        }

        // Guardamos físicamente
        m_view->guardarProyecto();
        return true; // Continuamos con la apertura del nuevo archivo
    }
    else if (respuesta == QMessageBox::Discard) {
        return true; // Continuamos descartando los cambios y abrimos el nuevo archivo
    }

    return false; // El usuario pulsó Cancelar, detenemos cualquier acción (No hace nada)
}

void Vista::dialogoComentario(){
    if (!m_view) return;

    // 1. Crear el contenedor del diálogo al vuelo (achatado y ancho)
    QDialog dlg(this);
    dlg.setWindowTitle(NOMBRE_APP);
    dlg.resize(750, 140); // Ajustamos la altura al reducirse a una línea

    // 2. Crear los componentes internos
    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    QLabel *lblInfo = new QLabel(tr("🧪 Descripción o notas asociadas a la molécula:"), &dlg);
    lblInfo->setStyleSheet("font-weight: bold; color: #38BDF8; font-size: 12px;");

    // 🎯 CAMBIO A QLINEEDIT: Una sola línea de entrada de texto
    QLineEdit *cajaTexto = new QLineEdit(&dlg);
    cajaTexto->setText(m_view->getComentario()); // Cargar nota actual
    cajaTexto->setPlaceholderText(tr("Escriba aquí detalles del cálculo, método químico o anotaciones..."));

    // Aplicamos tu paleta oscura con una fuente mayor (font-size: 14px)
    cajaTexto->setStyleSheet(
        "QLineEdit {"
        "    background-color: #030712;"
        "    color: #10B981;"
        "    font-family: 'Consolas', monospace;"
        "    font-size: 16px;" // ◄ Fuente incrementada
        "    border: 1px solid #374151;"
        "    border-radius: 6px;"
        "    padding: 6px 10px;"
        "}"
        );

    QDialogButtonBox *botones = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    botones->setStyleSheet(
        "QPushButton { background-color: #334155; color: #E2E8F0; border-radius: 4px; padding: 6px 14px; }"
        "QPushButton:hover { background-color: #475569; border-color: #38BDF8; }"
        "QPushButton[text*='OK'] { background-color: #0EA5E9; font-weight: bold; }"
        );

    // 3. Armar el diseño
    layout->addWidget(lblInfo);
    layout->addWidget(cajaTexto);
    layout->addWidget(botones);

    QObject::connect(botones, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(botones, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    // 4. Mostrar el diálogo en pantalla y capturar el resultado
    if (dlg.exec() == QDialog::Accepted) {
        m_view->setComentario(cajaTexto->text()); // ◄ .text() en lugar de .toPlainText()
    }
}

void Vista::configurarCalculoMopac(){
    if (!m_view) return;

    // 1. Crear el contenedor del diálogo nativo de MOPAC expandido
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Configurar Parámetros de MOPAC (Semiempírico)"));
    dlg.resize(480, 270);

    QVBoxLayout *layoutPrincipal = new QVBoxLayout(&dlg);
    layoutPrincipal->setContentsMargins(20, 20, 20, 20);
    layoutPrincipal->setSpacing(14);

    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(10);

    // Combo A: Método / Hamiltoniano
    QLabel *lblMetodo = new QLabel(tr("Método Químico:"), &dlg);
    QComboBox *comboMetodo = new QComboBox(&dlg);
    comboMetodo->addItems({"PM7", "PM6", "AM1", "RM1", "MNDO"});

    // =========================================================================
    // 🎯 NUEVO COMBO: ESTADO ELECTRÓNICO (FUNDAMENTAL S0 VS EXCITADO S1)
    // =========================================================================
    QLabel *lblEstadoElec = new QLabel(tr("Estado Electrónico:"), &dlg);
    QComboBox *comboEstadoElec = new QComboBox(&dlg);
    comboEstadoElec->addItem(tr("Singlete Fundamental (S₀)"), "");
    comboEstadoElec->addItem(tr("Primer Singlete Excitado (S₁ / C.I.)"), "C.I.=4 SINGLET EXCITED ROOT=2");

    // Combo C: Tipo de Tarea de simulación
    QLabel *lblTarea = new QLabel(tr("Tipo de Tarea:"), &dlg);
    QComboBox *comboTarea = new QComboBox(&dlg);
    comboTarea->addItem(tr("1.- Optimización Geométrica"), "OPT");
    comboTarea->addItem(tr("2.- Trayectoria Completa (Cálculo Dinámico)"), "OPT AUX LARGE");
    comboTarea->addItem(tr("3.- Modos Vibracionales / Frecuencias IR"), "AUX FORCE");

    // Campo D: Línea libre para keywords personalizadas del usuario
    QLabel *lblExtras = new QLabel(tr("Keywords Adicionales:"), &dlg);
    QLineEdit *txtExtras = new QLineEdit(&dlg);
    txtExtras->setPlaceholderText(tr("Ej: CHARGE=0 PRECISE EPS=78.4"));

    grid->addWidget(lblMetodo, 0, 0);     grid->addWidget(comboMetodo, 0, 1);
    grid->addWidget(lblEstadoElec, 1, 0); grid->addWidget(comboEstadoElec, 1, 1); // Inyección en la rejilla
    grid->addWidget(lblTarea, 2, 0);      grid->addWidget(comboTarea, 2, 1);
    grid->addWidget(lblExtras, 3, 0);     grid->addWidget(txtExtras, 3, 1);

    layoutPrincipal->addLayout(grid);

    QDialogButtonBox *botones = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    botones->setStyleSheet(
        "QPushButton { background-color: #334155; color: #E2E8F0; border-radius: 4px; padding: 6px 14px; }"
        "QPushButton:hover { background-color: #475569; border-color: #38BDF8; }"
        "QPushButton[text*='OK'] { background-color: #10B981; font-weight: bold; }"
        );
    layoutPrincipal->addWidget(botones);

    QObject::connect(botones, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(botones, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    // =================================================================================
    // 🔍 FASE DE APERTURA: Sincronizar el estado actual de la RAM en los controles
    // =================================================================================
    QString argsActuales = m_view->getArgumentosMopac().toUpper();

    // Sincronizar Combo Método
    for(int i = 0; i < comboMetodo->count(); ++i) {
        if(argsActuales.contains(comboMetodo->itemText(i))) {
            comboMetodo->setCurrentIndex(i);
            break;
        }
    }

    // Sincronizar el NUEVO Combo de Estado Electrónico buscando la presencia de C.I.
    if(argsActuales.contains("C.I.=") || argsActuales.contains("EXCITED")) {
        comboEstadoElec->setCurrentIndex(1); // Singlete Excitado S1
    } else {
        comboEstadoElec->setCurrentIndex(0); // Suelo S0
    }

    // Sincronizar Combo Tarea analizando prioridades estructurales
    if(argsActuales.contains("FORCE")) {
        comboTarea->setCurrentIndex(2); // AUX FORCE
    } else if(argsActuales.contains("LARGE") || argsActuales.contains("AUX")) {
        comboTarea->setCurrentIndex(1); // OPT AUX LARGE
    } else {
        comboTarea->setCurrentIndex(0); // OPT
    }

    // Aislar ÚNICAMENTE las palabras libres del usuario (Limpieza por tokens exactos)
    QStringList tokensActuales = argsActuales.split(' ', Qt::SkipEmptyParts);
    QStringList keywordsEstructurales = {"PM7", "PM6", "AM1", "RM1", "MNDO", "OPT", "FORCE", "AUX", "LARGE",
                                         "DIPOLE", "NOINTER", "C.I.=4", "SINGLET", "EXCITED", "ROOT=2"};
    QStringList extrasFiltrados;

    for (const QString& token : tokensActuales) {
        if (!keywordsEstructurales.contains(token)) {
            extrasFiltrados << token;
        }
    }
    txtExtras->setText(extrasFiltrados.join(" "));

    // =================================================================================
    // 🚀 FASE DE FINALIZACIÓN (SÓLO AL PULSAR OK): Creación limpia del comando final
    // =================================================================================
    if (dlg.exec() == QDialog::Accepted) {
        QStringList nuevosKeywords;

        // 1. Inyectar el método del combo (PM7, PM6, etc.)
        nuevosKeywords << comboMetodo->currentText();

        // 2. Inyectar la Tarea seleccionada
        QString tareaSeleccionada = comboTarea->currentData().toString();
        nuevosKeywords << tareaSeleccionada.split(' ', Qt::SkipEmptyParts);

        // 3. Inyectar el Estado Electrónico (Si es S1, acopla las palabras clave de Grimme/MOPAC)
        QString estadoKeywords = comboEstadoElec->currentData().toString();
        if (!estadoKeywords.isEmpty()) {
            nuevosKeywords << estadoKeywords.split(' ', Qt::SkipEmptyParts);
        }

        // 4. Inyectar las keywords manuales ingresadas en el cuadro de texto
        QString extrasTexto = txtExtras->text().trimmed().toUpper();
        if(!extrasTexto.isEmpty()) {
            nuevosKeywords << extrasTexto.split(' ', Qt::SkipEmptyParts);
        }

        // 5. Parámetros estáticos corporativos de acople requeridos por la aplicación
        nuevosKeywords << "DIPOLE" << "NOINTER";

        // Generación de la cadena unificada final libre de redundancias
        QString comandoFinal = nuevosKeywords.join(" ").simplified().trimmed();

        // Guardamos el comando en el cerebro del programa y actualizamos el cuadro de texto azul
        m_view->setArgumentosMopac(comandoFinal);
        ui->etArgumentos->setText(comandoFinal);

        m_tipoCalculo   = comboTarea->currentIndex();
        m_programa      = 0; // Mopac
    }
}

void Vista::configurarCalculoOrca(){
    if (!m_view) return;

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Configurar Parámetros de ORCA (DFT / TD-DFT)"));
    dlg.resize(520, 270);

    QVBoxLayout *layoutPrincipal = new QVBoxLayout(&dlg);
    layoutPrincipal->setContentsMargins(20, 20, 20, 20);
    layoutPrincipal->setSpacing(14);

    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(10);

    // =========================================================================
    // 📊 COMBO A: MÉTODOS / FUNCIONALES (ORDENADOS DE MENOR A MAYOR COSTE)
    // =========================================================================
    QLabel *lblMetodo = new QLabel(tr("Método / Funcional:"), &dlg);
    QComboBox *comboMetodo = new QComboBox(&dlg);

    // Clasificación rigurosa por exigencia de CPU y memoria RAM:
    comboMetodo->addItem("1. B97-3c (Ultra-rápido / Cribado Inicial)", "B97-3c");
    comboMetodo->addItem("2. BP86 (Gradiente Puro / Coste Bajo)", "BP86");
    comboMetodo->addItem("3. B3LYP (Híbrido Clásico / Coste Medio)", "B3LYP");
    comboMetodo->addItem("4. PBE0 (Híbrido Global / Coste Medio-Alto)", "PBE0");
    comboMetodo->addItem("5. wB97X-D4 (Separación de Rango / Exigente / ESIPT)", "wB97X-D4");
    comboMetodo->addItem("6. CAM-B3LYP (Corrección de Rango / Exigente)", "CAM-B3LYP");

    // =========================================================================
    // 📐 COMBO B: BASES ORBITALES (ORDENADAS DE MENOR A MAYOR TAMAÑO)
    // =========================================================================
    QLabel *lblBase = new QLabel(tr("Base Orbital (Basis Set):"), &dlg);
    QComboBox *comboBase = new QComboBox(&dlg);

    // Clasificación por número de funciones de base y funciones de polarización:
    comboBase->addItem("1. mini (Minimal / Rápida de Pruebas)", "mini");
    comboBase->addItem("2. def2-SVP (Double-Zeta / Coste Bajo)", "def2-SVP");
    comboBase->addItem("3. 6-31G(d) (Familia Pople / Coste Medio)", "6-31G(d)");
    comboBase->addItem("4. def2-TZVP (Triple-Zeta / Precisión Alta)", "def2-TZVP");
    comboBase->addItem("5. ma-def2-TZVP (Con Difusas / Exigente / Excitados S1)", "ma-def2-TZVP");

    // =========================================================================
    // 🎞️ COMBO C: TIPOS DE TAREA (TUS TRES FUNCIONES ESTRICTAS)
    // =========================================================================
    QLabel *lblTarea = new QLabel(tr("Tipo de Tarea:"), &dlg);
    QComboBox *comboTarea = new QComboBox(&dlg);
    comboTarea->addItem(tr("1.- Optimización Geométrica Estacionaria"), "Opt");
    comboTarea->addItem(tr("2.- Trayectoria Completa de Optimización"), "Opt");
    comboTarea->addItem(tr("3.- Frecuencias Analíticas e Espectro IR"), "Opt Freq");

    // Campo D: Keywords adicionales
    QLabel *lblExtras = new QLabel(tr("Keywords Adicionales:"), &dlg);
    QLineEdit *txtExtras = new QLineEdit(&dlg);
    txtExtras->setPlaceholderText(tr("Ej: TightSCF Grid4 CPCM(Solvent)"));

    grid->addWidget(lblMetodo, 0, 0);   grid->addWidget(comboMetodo, 0, 1);
    grid->addWidget(lblBase, 1, 0);     grid->addWidget(comboBase, 1, 1);
    grid->addWidget(lblTarea, 2, 0);    grid->addWidget(comboTarea, 2, 1);
    grid->addWidget(lblExtras, 3, 0);   grid->addWidget(txtExtras, 3, 1);

    layoutPrincipal->addLayout(grid);

    QDialogButtonBox *botones = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    botones->setStyleSheet(
        "QPushButton { background-color: #334155; color: #E2E8F0; border-radius: 4px; padding: 6px 14px; }"
        "QPushButton:hover { background-color: #475569; border-color: #38BDF8; }"
        "QPushButton[text*='OK'] { background-color: #0EA5E9; font-weight: bold; }"
        );
    layoutPrincipal->addWidget(botones);

    QObject::connect(botones, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(botones, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    // =================================================================================
    // 🔍 FASE DE APERTURA: Sincronizar con los argumentos de la RAM de View
    // =================================================================================
    QString argsActuales = m_view->getArgumentosOrca().toUpper();

    // 🎯 BLINDAJE DE PIES DE PLOMO: Si la RAM viene vacía por una importación limpia,
    // pre-configuramos los desplegables en el estándar de oro de tu estudio (PBE0 / def2-TZVP)
    if (argsActuales.isEmpty()) {
        comboMetodo->setCurrentIndex(3); // 4. PBE0 (Índice 3 en el combo)
        comboBase->setCurrentIndex(3);   // 4. def2-TZVP (Índice 3 en el combo)
        comboTarea->setCurrentIndex(0);  // 1.- Optimización Geométrica Estacionaria
    }
    else {
        // Si el usuario ya tenía parámetros configurados, ejecutamos tu lectura por tokens:
        for(int i = 0; i < comboMetodo->count(); ++i) {
            QString funcionalToken = comboMetodo->itemData(i).toString().toUpper();
            if(!funcionalToken.isEmpty() && argsActuales.contains(funcionalToken)) {
                comboMetodo->setCurrentIndex(i);
                break;
            }
        }

        for(int i = 0; i < comboBase->count(); ++i) {
            QString baseToken = comboBase->itemData(i).toString().toUpper();
            if(!baseToken.isEmpty() && argsActuales.contains(baseToken)) {
                comboBase->setCurrentIndex(i);
                break;
            }
        }

        if(argsActuales.contains("FREQ")) {
            comboTarea->setCurrentIndex(2); // Frecuencias
        } else {
            if (m_view->getEstado().hayTrayectoria) comboTarea->setCurrentIndex(1); // Trayectoria
            else comboTarea->setCurrentIndex(0); // Opt Estacionaria
        }
    }

    // Filtrado rígido de tokens fijos para limpiar el cuadro de extras del usuario
    QStringList tokensActuales = argsActuales.split(' ', Qt::SkipEmptyParts);
    QStringList keywordsFijas = {"B3LYP", "PBE0", "BP86", "PBE", "B97-3C", "WB97X-D4", "CAM-B3LYP",
                                 "DEF2-SVP", "DEF2-TZVP", "MA-DEF2-TZVP", "6-31G(D)", "MINI",
                                 "OPT", "FREQ", "!", "RIJCOSX", "DEF2/J", "DEF2-TZVP/JK", "DEF2-SVP/JK", "D4"};
    QStringList extrasFiltrados;

    for (const QString& token : tokensActuales) {
        if (!keywordsFijas.contains(token)) extrasFiltrados << token;
    }
    txtExtras->setText(extrasFiltrados.join(" "));

    // =================================================================================
    // 🚀 FASE DE FINALIZACIÓN (SÓLO AL PULSAR OK): Composición Automatizada Blindada
    // =================================================================================
    if (dlg.exec() == QDialog::Accepted) {
        QStringList nuevosKeywords;

        nuevosKeywords << "!"; // Exclamación reglamentaria de ORCA

        QString funcionalSel = comboMetodo->itemData(comboMetodo->currentIndex()).toString();
        QString baseSel      = comboBase->itemData(comboBase->currentIndex()).toString();

        // 1. Inyectamos Funcional y Base (B97-3c la omite por ser método compuesto integrado)
        nuevosKeywords << funcionalSel;
        if (funcionalSel != "B97-3c") {
            nuevosKeywords << baseSel;
        }

        // 2. Inyectar Tipo de Tarea
        nuevosKeywords << comboTarea->currentData().toString();

        // 3. Inyectar corrección de dispersión D4 automáticamente si aplica
        if (funcionalSel == "PBE0" || funcionalSel == "B3LYP") {
            nuevosKeywords << "D4";
        }

        // 4. Inyectar Keywords manuales del usuario
        QString extrasTexto = txtExtras->text().trimmed();
        if(!extrasTexto.isEmpty()) {
            nuevosKeywords << extrasTexto.split(' ', Qt::SkipEmptyParts);
        }

        // =====================================================================
        // 🎯 EL ESCUDO DE ACELERACIÓN NUMÉRICA AUTOMÁTICA (RIJCOSX)
        // =====================================================================
        // Si el funcional requiere cálculo de intercambio HF exacto (Híbridos),
        // inyectamos RIJCOSX acoplado con sus bases auxiliares óptimas correspondientes:
        if (!nuevosKeywords.contains("RIJCOSX", Qt::CaseInsensitive) &&
            (funcionalSel == "PBE0" || funcionalSel == "B3LYP" || funcionalSel == "CAM-B3LYP")) {

            nuevosKeywords << "def2/J";
            if (baseSel.contains("TZVP", Qt::CaseInsensitive)) {
                nuevosKeywords << "def2-TZVP/JK";
            } else {
                nuevosKeywords << "def2-SVP/JK";
            }
            nuevosKeywords << "RIJCOSX";
        }

        QString comandoFinal = nuevosKeywords.join(" ").simplified().trimmed();

        // Salvamos los argumentos en el cerebro y refrescamos la barra derecha
        m_view->setArgumentosOrca(comandoFinal);
        ui->etArgumentos->setText(comandoFinal);

        m_tipoCalculo   = comboTarea->currentIndex();
        m_programa      = 1; // ORCA
    }
}

void Vista::rellenarListaFrecuencias(){
    // 1. Limpieza total preventiva del componente visual
    ui->listaFrecuencias->clear();

    // 2. Recuperamos el vector completo de modos vibracionales que ya parseó el cerebro
    QVector<ModoVibracional> modos = m_view->getModosVibracionales();

    // 3. Recorremos los modos usando el índice 'i' para no perder la referencia real en la RAM
    for (int i = 0; i < modos.size(); ++i) {
        const ModoVibracional& modo = modos[i];

        // Construimos una cadena de texto elegante y legible para el usuario
        QString textoItem = QString("%1: %2 cm⁻¹ Int: %3")
                                .arg(modo.numero, 2) // Reserva mínimo 2 dígitos para mantener la alineación
                                .arg(modo.frecuencia, 7, 'f', 2)
                                .arg(modo.intensidad, 6, 'f', 2);


        // Creamos el elemento físico para la lista
        QListWidgetItem* item = new QListWidgetItem(textoItem);

        // 💡 TRUCO DE QT: Guardamos el índice real 'i' dentro del propio ítem (UserRole).
        // Como la lista visual tendrá menos elementos que el vector original (debido al filtro),
        // esto nos garantizará saber exactamente qué modo de la RAM pulsó el usuario más adelante.
        item->setData(Qt::UserRole, i);

        // Lo añadimos al widget
        ui->listaFrecuencias->addItem(item);
    }
}

void Vista::actualizarFrameVibracion(){
    if (m_idxModoActivo < 0) return;

    m_faseVibracion += 0.25f;
    if (m_faseVibracion > 2.0f * M_PI) {
        m_faseVibracion -= 2.0f * M_PI;
    }

    float valorSenoidal = qSin(m_faseVibracion);

    if (m_view) {
        // 1. El cerebro deforma m_listaAtomosTrabajo y emite 'actualizarLienzo'
        m_view->actualizarFaseVibracion(m_idxModoActivo, valorSenoidal, m_escalaVisual);
    }

    // 2. La vista se limita a repintar el viewport gráfico con los nuevos ítems de la escena
    if (m_lienzo) {
        m_lienzo->viewport()->update();
    }
}

void Vista::actualizarTituloYMetadatos(const Estado& estado) {
    if (!estado.estaEmpezado) {
        this->setWindowTitle(QString("%1 - Sin Proyecto").arg(NOMBRE_APP));
    } else {
        QString nombreArchivo = tr("Proyecto Nuevo");
        if (m_view && !m_view->getRutaActual().isEmpty()) {
            nombreArchivo = QFileInfo(m_view->getRutaActual()).fileName();
        }

        QString titulo = QString("%1 | %2").arg(NOMBRE_APP).arg(nombreArchivo);
        if (estado.estaModificado) {
            titulo += " *";
        }
        this->setWindowTitle(titulo);
    }
    ui->visorMolecular->setEnabled(estado.estaEmpezado);
}

void Vista::actualizarAccionesYMenus(const Estado& estado) {
    // Bloque inmutable (Siempre activos)
    ui->actionNuevo->setEnabled(true);
    ui->actionAbrir->setEnabled(true);
    ui->actionSalir->setEnabled(true);

    // Acciones de persistencia en disco
    ui->actionCerrar->setEnabled(estado.estaEmpezado);
    ui->actionGuardar->setEnabled(estado.estaEmpezado && !estado.estaVacio);
    ui->actionGuardarComo->setEnabled(estado.estaEmpezado && !estado.estaVacio);

    // Historial y Edición Estructural
    ui->actionDeshacer->setEnabled(estado.hayDeshacer);
    ui->actionRehacer->setEnabled(estado.hayRehacer);
    ui->actionComentarioProyecto->setEnabled(estado.estaEmpezado && !estado.estaVacio);
    ui->actionLimpiarLienzo->setEnabled(estado.estaEmpezado && !estado.estaVacio);

    // =========================================================================
    // 🧮 GESTIÓN DE ACCIONES PARA EL NUEVO MENÚ "CÁLCULO QM"
    // =========================================================================
    // El botón de abortar y la acción del menú superior responden coordinadamente
    ui->actionDetenerSimulacion->setEnabled(estado.estaEmpezado && estado.hayProceso);
    ui->btnAbortar->setEnabled(estado.estaEmpezado && estado.hayProceso);

    // 🛡️ BLINDAJE DE SEGURIDAD FOTOQUÍMICO:
    // Si ya hay un cálculo corriendo en /tmp, bloqueamos los disparadores de lanzamiento
    // y la configuración de parámetros para evitar colisiones críticas de archivos.
    ui->btnLanzar->setEnabled(estado.estaEmpezado && !estado.hayProceso);
    ui->actionLanzar_Simulacion_Cuantica->setEnabled(estado.estaEmpezado && !estado.hayProceso);
    ui->actionConfigurar_Parametros_QM->setEnabled(estado.estaEmpezado && !estado.hayProceso);
}

void Vista::actualizarControlesEdicion(const Estado& estado) {
    if (estado.estaEmpezado) {
        ui->barraIzda->setEnabled(true);
        ui->btnSeleccion->setChecked(estado.modoEditor == ModoEditor::ModoSeleccion);
        ui->btnModoEnlace->setChecked(estado.modoEditor == ModoEditor::ModoCrearEnlace);
        ui->btnModoRotar->setChecked(estado.modoEditor == ModoEditor::ModoRotacion3D);
    } else {
        ui->barraIzda->setEnabled(false);
        m_grupoIzdo->setExclusive(false);
        if (m_grupoIzdo->checkedButton()) {
            m_grupoIzdo->checkedButton()->setChecked(false);
        }
        m_grupoIzdo->setExclusive(true);
    }
}

void Vista::actualizarPanelResultados(const Estado& estado) {
    int numeroAtomos = m_view ? m_view->getListaAtomos().count() : 0;
    bool usarPanelDerecho = (estado.estaEmpezado && numeroAtomos > 1);
    ui->barraDerecha->setEnabled(usarPanelDerecho);

    if (!usarPanelDerecho) {
        ui->listaFrecuencias->clear();
        ui->lblTituloResultados->setText(tr("📊 RESULTADOS Y ESTADO DEL CÁLCULO"));
        return;
    }

    if (m_view) {
        if(ui->comboSoftware->currentIndex()==0){
            ui->etArgumentos->setText(m_view->getArgumentosMopac());
        }
        else{
            ui->etArgumentos->setText(m_view->getArgumentosOrca());
        }
    }

    ui->listaFrecuencias->setVisible(estado.hayFrecuencias);
    ui->listaFrecuencias->setEnabled(!estado.hayProceso);

    for (int i = 0; i < ui->formLayoutResultados->count(); ++i) {
        if (QWidget* w = ui->formLayoutResultados->itemAt(i)->widget()) {
            w->setVisible(!estado.hayFrecuencias);
        }
    }

    // 🎯 REINICIO ADAPTATIVO MIENTRAS SE EJECUTA EL PROCESO (Filas eliminadas)
    if (estado.hayProceso) {
        ui->lblTituloResultados->setText(tr("⏳ CÁLCULO EN PROCESO..."));
        ui->listaFrecuencias->clear();

        ui->lblEnergia->setText("-- u.a.");
        ui->lblCalorFormacion->setText("-- kcal/mol");
        ui->lblIonizacion->setText("-- eV");
        ui->lblDipolo->setText("-- D");
        ui->lblTiempoCpu->setText("-- s");
    }
    else if (estado.hayFrecuencias) {
        ui->lblTituloResultados->setText(tr("📈 MODOS VIBRACIONALES DETECTADOS"));
        rellenarListaFrecuencias();
    } else {
        ui->lblTituloResultados->setText(tr("📊 RESULTADOS DEL CÁLCULO"));
        if (m_view) {
            ValoresCalculo valores = m_view->getValoresCalculo();
            // Eliminadas las asignaciones a lblEstado y lblSimetria
            ui->lblEnergia->setText(valores.energia);
            ui->lblCalorFormacion->setText(valores.calorFormacion);
            ui->lblIonizacion->setText(valores.ionizacion);
            ui->lblDipolo->setText(valores.dipolo);
            ui->lblTiempoCpu->setText(valores.tiempoCpu);
        }
    }
}

void Vista::actualizarGraficaEInferior(const Estado& estado) {
    int numeroAtomos = m_view ? m_view->getListaAtomos().count() : 0;
    bool usarPanelDerecho = (estado.estaEmpezado && numeroAtomos > 1);
    ui->wGrafica->setEnabled(usarPanelDerecho);

    // =========================================================================
    // 🧹 PASO DE CONTROL: LIMPIEZA TOTAL PREVENTIVA (INCLUYE SET MENSAJES)
    // =========================================================================
    stopAnimaciones(estado);

    bool esperandoSegundaFase = m_view ? m_view->property("esperandoForceSecuencial").toBool() : false;
    bool tieneConvergenciaNormal = (estado.estaOptimizado && m_view && !m_view->getCalores().isEmpty() && !esperandoSegundaFase);

    // =========================================================================
    // 🎞️ CASO A: REPRODUCCIÓN DINÁMICA DE LA PELÍCULA (.AUX / _TRJ.XYZ)
    // =========================================================================
    if (estado.hayTrayectoria) {
        ui->lblTituloFreq->setText(tr("🎞️ PERFIL DE TRAYECTORIA ENERGÉTICA"));
        ui->wGrafica->setVisible(true);
        ui->frameTrayectoria->setVisible(true);

        if (m_view) {
            ui->wGrafica->setDatos(m_view->getPelicula());
            ui->wGrafica->setPasoActual(m_view->getFotogramaActual());
        }

        ui->sliderPasos->blockSignals(true);
        ui->sliderPasos->setRange(0, m_view->getNumeroFotogramas() - 1);
        ui->sliderPasos->setValue(m_view->getFotogramaActual());
        ui->sliderPasos->blockSignals(false);
        return;
    }

    // =========================================================================
    // 📈 CASO B: ESPECTRO DE ABSORCIÓN INFRARROJA ACTIVO (IR)
    // =========================================================================
    if (estado.hayFrecuencias) {
        ui->lblTituloFreq->setText(tr("📈 ESPECTRO DE ABSORCIÓN INFRARROJA (IR)"));
        ui->wGrafica->setVisible(true);

        if (m_view) {
            ui->wGrafica->setDatosEspectro(m_view->getModosVibracionales());
            QVector<ModoVibracional> modos = m_view->getModosVibracionales();

            if (!modos.isEmpty() && estado.modoEditor == ModoEditor::ModoRotacion3D) {
                if (ui->listaFrecuencias->count() == 0) {
                    rellenarListaFrecuencias();
                }

                if (ui->listaFrecuencias->count() > 0) {
                    ui->listaFrecuencias->setCurrentRow(0);
                    QListWidgetItem* primerItem = ui->listaFrecuencias->item(0);
                    m_idxModoActivo = primerItem->data(Qt::UserRole).toInt();
                } else {
                    m_idxModoActivo = 0;
                }

                if (m_idxModoActivo >= 0 && m_idxModoActivo < modos.size()) {
                    ui->wGrafica->setModoActivo(modos[m_idxModoActivo].numero);
                }

                m_faseVibracion = 0.0f;
                m_timerVibracion->start(30);
            }
        }
        return;
    }

    // =========================================================================
    // 📊 CASO C: CURVA DE CONVERGENCIA ESTÁTICA
    // =========================================================================
    if (tieneConvergenciaNormal) {
        ui->lblTituloFreq->setText(tr("📊 CURVA DE CONVERGENCIA ENERGÉTICA"));
        ui->wGrafica->setVisible(true);

        if (m_view) {
            ui->wGrafica->setDatosCalores(m_view->getCalores());
            ui->wGrafica->setPasoActual(m_view->getCalores().size() - 1);
        }
        return;
    }

    // =========================================================================
    // 💤 CASO D: REPOSO ABSOLUTO O TRANSICIÓN INTERMEDIA
    // =========================================================================
    ui->lblTituloFreq->setText(esperandoSegundaFase ? tr("⏳ OPTIMIZACIÓN LISTA. CALCULANDO MODOS VIBRACIONALES...") : "");
    ui->wGrafica->setVisible(true);

    QVector<ModoVibracional> vacio;
    ui->wGrafica->setDatosEspectro(vacio);


    // =========================================================================
    // 💬 7. BARRA DE CONSEJOS EN EL SUELO DE LA INTERFAZ
    // =========================================================================
    if (!estado.estaEmpezado) {
        m_lblMesaje->setText(tr(" Listo. Cree un nuevo proyecto o abra un archivo para comenzar."));
    } else {
        QString txt = "";
        switch (estado.modoEditor) {
        case ModoSeleccion:   txt = tr(" Modo Selección | Use el puntero para interactuar con los átomos."); break;
        case ModoCrearEnlace: txt = tr(" Modo Enlace 🔗 | Seleccione dos átomos para enlazarlos."); break;
        case ModoRotacion3D:  txt = tr(" Modo Rotación 3D 🔄 | Arrastre el lienzo para orbitar la molécula."); break;
        case ModoDibujo:      txt = tr(" Modo Dibujo 🧪 | Añadiendo elemento: %1 (Clic en lienzo para colocar)").arg(m_view->getAtomoActivo()); break;
        }
        m_lblMesaje->setText(txt);
    }
}


///======================================================================================
// SLOTS PRIVADOS de Botones
//======================================================================================
void Vista::on_actionSalir_triggered(){
    close(); // Cierra limpiamente la ventana principal y finaliza el proceso
}

void Vista::on_actionNuevo_triggered(){
    if(sePuedeCerrar()){
        m_view->nuevoProyecto();
    }
}

void Vista::on_actionCerrar_triggered(){
    if(sePuedeCerrar()){
        m_view->cerrarProyecto();
    }
}

void Vista::on_actionAbrir_triggered(){
    if (!m_view) return;

    // 0. Antes de abrir, verificamos si hay cambios sin guardar en el lienzo actual
    if (!sePuedeCerrar()) {
        return; // El usuario canceló la operación en el cuadro de diálogo
    }

    // 1. La Vista se encarga del cuadro de diálogo nativo de Qt
    QString ruta = QFileDialog::getOpenFileName(
        this,
        tr("Abrir Proyecto VisMol"),
        "",
        tr("Archivos de VisMol (*.vismol)")
        );

    // 2. Si el usuario seleccionó un archivo, se delega al cerebro (View)
    if (!ruta.isEmpty()) {
        m_view->abrirProyecto(ruta);
    }
}

void Vista::on_actionGuardar_triggered(){
    if (!m_view) return;

    // 1. Comprobar si existe ruta en el cerebro. Si no existe, pedirla.
    if (m_view->getRutaActual().isEmpty()) {
        QString ruta = QFileDialog::getSaveFileName(
            this,
            tr("Guardar Proyecto Como"),
            "",
            tr("Archivos de VisMol (*.vismol)")
            );

        if (ruta.isEmpty()) return; // El usuario canceló el diálogo

        // Aseguramos la extensión nativa
        if (!ruta.endsWith(".vismol", Qt::CaseInsensitive)) {
            ruta += ".vismol";
        }

        // 2. Llamamos al método del cerebro con la nueva ruta
        m_view->setRutaActual(ruta);
        m_view->guardarProyecto();
    }
    else {
        // Si ya tiene ruta asignada, guardamos directamente sobre ella
        m_view->guardarProyecto();
    }
}

void Vista::on_btnCarbono_clicked(){
    m_view->setElementoActivo("C");
}

void Vista::on_btnHidrogeno_clicked(){
    m_view->setElementoActivo("H");
}

void Vista::on_btnOxigeno_clicked(){
    m_view->setElementoActivo("O");
}

void Vista::on_btnNitrogeno_clicked(){
    m_view->setElementoActivo("N");
}

void Vista::on_btnAzufre_clicked(){
    m_view->setElementoActivo("S");
}

void Vista::on_btnSeleccion_clicked(){
    m_view->setModoEditor(ModoSeleccion);
}

void Vista::on_btnModoEnlace_clicked(){
    m_view->setModoEditor(ModoCrearEnlace);
}

void Vista::on_btnModoRotar_clicked(){
    m_view->setModoEditor(ModoRotacion3D);
}

void Vista::on_actionDeshacer_triggered(){
    m_view->deshacer();
}

void Vista::on_actionRehacer_triggered(){
    m_view->rehacer();
}

void Vista::on_actionLimpiarLienzo_triggered(){
    m_view->limpiarLienzo();
}

void Vista::on_actionComentarioProyecto_triggered(){
    dialogoComentario();
}

void Vista::on_actionGuardarComo_triggered(){
    if (!m_view) return;

    QString ruta = QFileDialog::getSaveFileName(
        this,
        tr("Guardar Proyecto Como"),
        "",
        tr("Archivos de VisMol (*.vismol)")
        );

    if (!ruta.isEmpty()) {
        if (!ruta.endsWith(".vismol", Qt::CaseInsensitive)) {
            ruta += ".vismol";
        }
        m_view->setRutaActual(ruta);
        m_view->guardarProyecto();
    }
}

void Vista::on_btnConfigurarParametros_clicked(){
    if (!m_view) return;

    if(ui->comboSoftware->currentIndex() == 0){
        configurarCalculoMopac();
    }
    else{
        configurarCalculoOrca();  // Cambiado para desplegar el panel ab initio de ORCA
    }
}

void Vista::on_btnLanzar_clicked(){
    m_view->lanzarCalculoQm(m_programa, m_tipoCalculo);
}

void Vista::on_btnPasoAtras_clicked(){
    if (!m_view || !m_view->getEstado().hayTrayectoria) return;

    // Avanzamos el cabezal en el cerebro
    m_view->fotogramaAtras();

    // Sincronizamos la posición de la barra de desplazamiento visual
    ui->sliderPasos->blockSignals(true);
    ui->sliderPasos->setValue(m_view->getFotogramaActual());
    ui->sliderPasos->blockSignals(false);

    ui->wGrafica->setPasoActual(m_view->getFotogramaActual());
}

void Vista::on_btnPasoAdelante_clicked(){
    if (!m_view || !m_view->getEstado().hayTrayectoria) return;

    // Avanzamos el cabezal en el cerebro
    m_view->fotogramaAdelante();

    // Sincronizamos la posición de la barra de desplazamiento visual
    ui->sliderPasos->blockSignals(true);
    ui->sliderPasos->setValue(m_view->getFotogramaActual());
    ui->sliderPasos->blockSignals(false);

    // Actualizar el marcador visual en la gráfica
    ui->wGrafica->setPasoActual(m_view->getFotogramaActual());
}

void Vista::on_listaFrecuencias_itemClicked(QListWidgetItem *item){
    if (!item || !m_timerVibracion || !m_view) return;

    m_timerVibracion->stop();

    // 1. m_idxModoActivo sigue guardando el índice de la RAM para la simulación de átomos
    m_idxModoActivo = item->data(Qt::UserRole).toInt();

    // 2. Recuperamos el vector original para extraer el número de modo real de este ítem
    QVector<ModoVibracional> modos = m_view->getModosVibracionales();
    if (m_idxModoActivo >= 0 && m_idxModoActivo < modos.size()) {
        int numeroModoReal = modos[m_idxModoActivo].numero;

        // 🎯 SINCRO PERFECTA: Le pasamos el número identificativo a la gráfica (ej: 19)
        ui->wGrafica->setModoActivo(numeroModoReal);
    }

    m_faseVibracion = 0.0f;
    m_timerVibracion->start(30);
}

void Vista::on_sliderPasos_valueChanged(int value){
    if(!m_view || !m_view->getEstado().hayTrayectoria) return;

    m_view->setFotogramaActual(value);
    ui->wGrafica->setPasoActual(value);
}

void Vista::on_btnPlayAnimacion_clicked(){
    if (!m_view || !m_view->getEstado().hayTrayectoria) return;

    if (!m_timerAnimacion->isActive()) {
        // 🔄 Si el reproductor llegó al final de la animación, lo reseteamos al inicio antes de reproducir
        if (m_view->getFotogramaActual() >= m_view->getNumeroFotogramas() - 2) {
            ui->sliderPasos->setValue(0);
        }

        m_timerAnimacion->start();
        ui->btnPlayAnimacion->setText(tr("⏸️ Pausa"));
    }
    else {
        m_timerAnimacion->stop();
        ui->btnPlayAnimacion->setText(tr("▶️ Play"));
    }
}

void Vista::on_comboSoftware_activated(int index){
    if (!m_view) return;

    // 1. Forzamos de forma manual un disparo de actualización óptica en el visor de argumentos
    if (index == 0) {
        ui->etArgumentos->setText(m_view->getArgumentosMopac());
    } else {
        ui->etArgumentos->setText(m_view->getArgumentosOrca());
    }

    // 2. Refrescamos el resto del panel derecho respetando el proyecto actual
    actualizarPanelResultados(m_view->getEstado());
}

void Vista::on_btnAbortar_clicked(){
    if (m_view) {
        m_view->stopProceso(); // El cerebro fulmina el PID y libera los paneles
    }
}

void Vista::on_actionDetenerSimulacion_triggered(){
    if (m_view) {
        m_view->stopProceso(); // El cerebro fulmina el PID y libera los paneles
    }
}

void Vista::on_actionGeometria_Puntual_triggered(){
    if (!m_view) return;

    QString filtro = tr("Coordenadas Finales de ORCA (*.xyz)");
    QString ruta = QFileDialog::getOpenFileName(this, tr("Importar Geometría Puntual de ORCA"), "", filtro);
    if (ruta.isEmpty()) return;

    // Ya no le pasamos el "1". Solo la ruta y el código de cálculo (0 = Opt)
    m_view->importarORCA(ruta, 0);

}

void Vista::on_actionTrayectoria_Optimizacion_triggered(){
    if (!m_view) return;

    // El explorador filtra de forma nativa por el archivo de película de pasos de ORCA
    QString filtro = tr("Trayectorias de Optimización ORCA (*trj.xyz)");

    QString ruta = QFileDialog::getOpenFileName(this, tr("Importar Trayectoria de ORCA"), "", filtro);
    if (ruta.isEmpty()) return;

    // Pasamos la ruta limpia y el código de cálculo (1 = Trayectoria)
    m_view->importarORCA(ruta, 1);

}

void Vista::on_actionCalculo_de_frecuencias_triggered(){
    if (!m_view) return;

    // Bloqueamos la entrada a matrices hessianas de ORCA para evitar lecturas cruzadas
    QString filtro = tr("Matrices Hessianas de ORCA (*.hess)");

    QString ruta = QFileDialog::getOpenFileName(this, tr("Importar Modos Vibracionales de ORCA"), "", filtro);
    if (ruta.isEmpty()) return;

    // Pasamos la ruta limpia y el código de cálculo (2 = Frecuencias / Freq)
    m_view->importarORCA(ruta, 2);


}

void Vista::on_actionExportar_archivo_ORCA_triggered(){
    if (!m_view) return;

    // 1. Abrimos el explorador de archivos nativo de Qt en modo "Guardar"
    QString filtro = tr("Archivos de Entrada ORCA (*.inp)");
    QString ruta = QFileDialog::getSaveFileName(
        this,
        tr("Exportar Input para ORCA"),
        "",
        filtro
        );

    // Si el usuario cancela o cierra la ventana, salimos pacíficamente
    if (ruta.isEmpty()) return;

    // 2. Blindaje de extensión: aseguramos que termine estrictamente en .inp
    if (!ruta.endsWith(".inp", Qt::CaseInsensitive)) {
        ruta += ".inp";
    }

    // 3. Pasamos la ruta limpia al cerebro para que realice la escritura doble
    m_view->exportarOrca(ruta);

}

void Vista::on_actionArchivo_xyz_ORCA_triggered(){
    if (!m_view) return;

    // El explorador filtra estrictamente por el archivo .xyz temporal que dejó ORCA a medias
    QString filtro = tr("Geometría Intermedia de ORCA (*.xyz)");
    QString ruta = QFileDialog::getOpenFileName(this, tr("Rescatar Cálculo Incompleto (ORCA)"), "", filtro);
    if (ruta.isEmpty()) return;

    // Llamamos a la nueva pasarela del cerebro con el código de cálculo 0 (Estático/Opt)
    m_view->cargarXyzORCA(ruta, 0);
}

void Vista::on_actionConfigurar_Parametros_QM_triggered(){
    on_btnConfigurarParametros_clicked();
}

void Vista::on_actionLanzar_Simulacion_Cuantica_triggered(){
    on_btnLanzar_clicked();
}

void Vista::on_actionExportar_Input_para_MOPAC_triggered(){
    if (!m_view) return;

    QString filtro = tr("Archivos de Entrada MOPAC (*.mop)");
    QString ruta = QFileDialog::getSaveFileName(this, tr("Exportar Input para MOPAC"), "", filtro);

    if (ruta.isEmpty()) return;

    if (!ruta.endsWith(".mop", Qt::CaseInsensitive)) {
        ruta += ".mop";
    }

    m_view->exportarMopac(ruta);

}

void Vista::on_actionImportar_opt_triggered(){
    if (!m_view) return;

    // 🎯 REPARACIÓN DE PIES DE PLOMO: Eliminamos el comodín *.out del filtro visual
    QString filtro = tr("Resumen de Archivos MOPAC (*.arc)");

    QString ruta = QFileDialog::getOpenFileName(
        this,
        tr("Importar Geometría Estacionaria (MOPAC .arc)"),
        "",
        filtro
        );

    if (!ruta.isEmpty()) {
        m_view->importarMopac(ruta, 0); // Tu cerebro lo procesará de forma limpia y directa
    }
}


void Vista::on_actionimportar_Trayectoria_triggered(){
    if (!m_view) return;
    QString filtro = tr("Archivos Auxiliares MOPAC (*.aux)");
    QString ruta = QFileDialog::getOpenFileName(this, tr("Importar Trayectoria Dinámica (MOPAC)"), "", filtro);
    if (!ruta.isEmpty()) m_view->importarMopac(ruta, 1);
}

void Vista::on_actionImportar_Frq_triggered(){
    if (!m_view) return;
    QString filtro = tr("Frecuencias Vibracionales MOPAC (*.out)");
    QString ruta = QFileDialog::getOpenFileName(this, tr("Importar Espectro Vibracional (MOPAC)"), "", filtro);
    if (!ruta.isEmpty()) m_view->importarMopac(ruta, 2);
}

