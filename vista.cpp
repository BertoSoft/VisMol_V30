#include "vista.h"
#include "ui_vista.h"

#include <QMessageBox>
#include <QDate>
#include <QTime>
#include <QTimer>
#include <QInputDialog>
#include <QFileInfo>
#include <QFileDialog>

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
}

//======================================================================================
// Funciones SLOT Publicos
//======================================================================================
void Vista::actualizarUi(Estado estado){
    // =================================================================================
    // 1. VENTANA PRINCIPAL, METADATOS Y TÍTULO DINÁMICO
    // =================================================================================
    if (!estado.estaEmpezado) {
        this->setWindowTitle(QString("%1 - Sin Proyecto").arg(NOMBRE_APP));
    }
    else {
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

    // Habilitar/Deshabilitar área de simulación principal
    ui->visorMolecular->setEnabled(estado.estaEmpezado);


    // =================================================================================
    // 2. ACCIONES globales, MENÚS Y BOTONES DE CONTROL (ACCIONES SÍ/NO)
    // =================================================================================
    // Bloque base (Siempre activos)
    ui->actionNuevo->setEnabled(true);
    ui->actionAbrir->setEnabled(true);
    ui->actionSalir->setEnabled(true);

    // Menú Archivo dependiente de estado activo
    ui->actionCerrar->setEnabled(estado.estaEmpezado);
    ui->actionGuardar->setEnabled(estado.estaEmpezado && !estado.estaVacio);
    ui->actionGuardarComo->setEnabled(estado.estaEmpezado && !estado.estaVacio);

    // Menú Edición e Historial
    ui->actionDeshacer->setEnabled(estado.hayDeshacer);
    ui->actionRehacer->setEnabled(estado.hayRehacer);
    ui->actionComentarioProyecto->setEnabled(estado.estaEmpezado && !estado.estaVacio);
    ui->actionLimpiarLienzo->setEnabled(estado.estaEmpezado && !estado.estaVacio);

    // Procesos cuánticos y abortar
    ui->actionDetenerSimulacion->setEnabled(estado.estaEmpezado && estado.hayProceso);
    ui->btnAbortar->setEnabled(estado.estaEmpezado && estado.hayProceso);


    // =================================================================================
    // 3. BARRA LATERAL IZQUIERDA (HERRAMIENTAS DE EDICIÓN Y ATOMOS)
    // =================================================================================
    if (estado.estaEmpezado) {
        ui->barraIzda->setEnabled(true);
        ui->btnSeleccion->setChecked(estado.modoEditor == ModoEditor::ModoSeleccion);
        ui->btnModoEnlace->setChecked(estado.modoEditor == ModoEditor::ModoCrearEnlace);
        ui->btnModoRotar->setChecked(estado.modoEditor == ModoEditor::ModoRotacion3D);
    }
    else {
        ui->barraIzda->setEnabled(false);
        m_grupoIzdo->setExclusive(false);
        if (m_grupoIzdo->checkedButton()) {
            m_grupoIzdo->checkedButton()->setChecked(false);
        }
        m_grupoIzdo->setExclusive(true);
    }


    // =================================================================================
    // 4. BARRA LATERAL DERECHA SUPERIOR (INTERCAMBIO: RESULTADOS VS LISTA FREQUENCIAS)
    // =================================================================================
    int numeroAtomos = m_view ? m_view->getListaAtomos().count() : 0;
    bool usarPanelDerecho = (estado.estaEmpezado && numeroAtomos > 1);
    ui->barraDerecha->setEnabled(usarPanelDerecho);

    if (usarPanelDerecho) {
        if (m_view) {
            ui->etArgumentos->setText(m_view->getArgumentos());
        }

        // 🎯 LÓGICA DE INTERCAMBIO ATÓMICO INTERNO
        if (estado.hayFrecuencias) {
            // Caso Espectro IR: Ocultamos los componentes individuales de la rejilla de datos
            ui->lblTituloResultados->setText(tr("📈 MODOS VIBRACIONALES DETECTADOS"));

            // Ocultamos todos los elementos del formLayout uno a uno de forma segura
            for (int i = 0; i < ui->formLayoutResultados->count(); ++i) {
                if (QWidget* w = ui->formLayoutResultados->itemAt(i)->widget()) {
                    w->setVisible(false);
                }
            }

            // Mostramos la lista de frecuencias en el espacio superior liberado
            ui->listaFrecuencias->setVisible(true);
            ui->listaFrecuencias->setEnabled(!estado.hayProceso);
        }
        else {
            // Caso Optimización / Trayectoria: Restauramos el panel estándar
            ui->lblTituloResultados->setText(tr("📊 RESULTADOS Y ESTADO DEL CÁLCULO"));
            ui->listaFrecuencias->setVisible(false); // Ocultamos la lista de la zona alta

            // Volvemos a visibilizar los labels individuales del formulario
            for (int i = 0; i < ui->formLayoutResultados->count(); ++i) {
                if (QWidget* w = ui->formLayoutResultados->itemAt(i)->widget()) {
                    w->setVisible(true);
                }
            }

            // Sincronizamos los datos desde la RAM del cerebro si no hay simulación activa
            if (!estado.hayProceso && m_view) {
                ValoresCalculo valores = m_view->getValoresCalculo();
                ui->lblEstado->setText(estado.hayTrayectoria ? tr("Paso de Optimización %1").arg(m_view->getFotogramaActual() + 1) : tr("Listo / Estacionario"));
                ui->lblEnergia->setText(valores.energia);
                ui->lblCalorFormacion->setText(valores.calorFormacion);
                ui->lblIonizacion->setText(valores.ionizacion);
                ui->lblSimetria->setText(valores.simetria);
                ui->lblDipolo->setText(valores.dipolo);
                ui->lblTiempoCpu->setText(valores.tiempoCpu);
            }
        }
    }

    // =================================================================================
    // 5. PANEL INFERIOR DERECHO (WIDGET GRÁFICA ATÓMICO: CURVAS VS ESPECTRO IR)
    // =================================================================================
    bool tieneConvergenciaNormal = (estado.estaOptimizado && m_view && !m_view->getCalores().isEmpty());
    ui->wGrafica->setEnabled(usarPanelDerecho);

    if (estado.hayTrayectoria) {
        // Escenario A: Modo Película / Trayectoria Dinámica activa
        ui->lblTituloFreq->setText(tr("🎞️ PERFIL DE TRAYECTORIA ENERGÉTICA"));
        ui->wGrafica->setVisible(true);
        ui->frameTrayectoria->setVisible(true);

        if (m_view) {
            ui->wGrafica->setDatos(m_view->getPelicula());
            ui->wGrafica->setPasoActual(m_view->getFotogramaActual());
        }
    }
    else if (tieneConvergenciaNormal) {
        // Escenario B: Optimización convencional (Curva estática del .out)
        if (m_timerAnimacion && m_timerAnimacion->isActive()) {
            m_timerAnimacion->stop();
            ui->btnPlayAnimacion->setText(tr("▶️ Play"));
        }
        ui->frameTrayectoria->setVisible(false);

        ui->lblTituloFreq->setText(tr("📊 CURVA DE CONVERGENCIA ENERGÉTICA"));
        ui->wGrafica->setVisible(true);

        if (m_view) {
            ui->wGrafica->setDatosCalores(m_view->getCalores());
            ui->wGrafica->setPasoActual(m_view->getCalores().size() - 1);
        }
    }
    else if (estado.hayFrecuencias) {
        // Escenario C: Espectroscopia (WIDGET COMPLETO ABAJO PARA EL ESPECTRO IR)
        if (m_timerAnimacion && m_timerAnimacion->isActive()) {
            m_timerAnimacion->stop();
            ui->btnPlayAnimacion->setText(tr("▶️ Play"));
        }
        ui->frameTrayectoria->setVisible(false);

        ui->lblTituloFreq->setText(tr("📈 ESPECTRO DE ABSORCIÓN INFRARROJA (IR)"));
        ui->wGrafica->setVisible(true); // Se queda expandido ocupando toda la base

        if (m_view) {
            // Aquí acoplaremos mañana el motor matemático de transmitancia
            // ui->wGrafica->setDatosEspectro(m_view->getNumerosOnda(), m_view->getIntensidades());
        }
    }
    else {
        // Escenario D: Reposo / Vacío (Mantenemos la gráfica limpia para conservar el color oscuro)
        if (m_timerAnimacion && m_timerAnimacion->isActive()) {
            m_timerAnimacion->stop();
            ui->btnPlayAnimacion->setText(tr("▶️ Play"));
        }
        ui->frameTrayectoria->setVisible(false);

        ui->lblTituloFreq->setText("");
        ui->wGrafica->setVisible(true);

        QVector<float> vacio;
        ui->wGrafica->setDatosCalores(vacio);
    }

    // =================================================================================
    // 6. SINCRONIZACIÓN DE LA LÍNEA DE TIEMPO (SLIDER DE PASOS)
    // =================================================================================
    if (estado.estaEmpezado && estado.hayTrayectoria && m_view) {
        ui->sliderPasos->blockSignals(true);
        ui->sliderPasos->setRange(0, m_view->getNumeroFotogramas() - 1);
        ui->sliderPasos->setValue(m_view->getFotogramaActual());
        ui->sliderPasos->blockSignals(false);
    }
    else {
        ui->sliderPasos->setRange(0, 0);
        ui->sliderPasos->setValue(0);
    }


    // =================================================================================
    // 7. BARRA DE ESTADO INFERIOR (CONSEJOS AL USUARIO)
    // =================================================================================
    if (!estado.estaEmpezado) {
        m_lblMesaje->setText(tr(" Listo. Cree un nuevo proyecto o abra un archivo para comenzar."));
    }
    else {
        QString txt = "";
        switch (estado.modoEditor) {
        case ModoSeleccion:
            txt = tr(" Modo Selección | Use el puntero para interactuar con los átomos.");
            break;
        case ModoCrearEnlace:
            txt = tr(" Modo Enlace 🔗 | Seleccione dos átomos para enlazarlos.");
            break;
        case ModoRotacion3D:
            txt = tr(" Modo Rotación 3D 🔄 | Arrastre el lienzo para orbitar la molécula.");
            break;
        case ModoDibujo:
            txt = tr(" Modo Dibujo 🧪 | Añadiendo elemento: %1 (Clic en lienzo para colocar)").arg(m_view->getAtomoActivo());
            break;
        }
        m_lblMesaje->setText(txt);
    }
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
        "}"
        // Selector específico para los valores mutables de los resultados:
        "QLabel#lblEstado, QLabel#lblEnergia, QLabel#lblCalorFormacion, "
        "QLabel#lblIonizacion, QLabel#lblSimetria, QLabel#lblDipolo, QLabel#lblTiempoCpu {"
        "    color: #10B981;" // Forzamos el color verde brillante terminal para los datos variables
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    font-size: 13px;"
        "}"
        "QListWidget#listaFrecuencias {"
        "    background-color: #030712;"
        "    color: #10B981;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    border: 1px solid #1F2937;"
        "    border-radius: 4px;"
        "}"

        // 🎯 REQUISITO: Estilo adaptativo para la lista cuando el panel está inactivo
        "QListWidget#listaFrecuencias:disabled {"
        "    background-color: #1E293B;" /* Mismo fondo exacto que barraDerecha */
        "    border: 1px solid #1E293B;" /* Difuminamos el borde para que se fusione */
        "    color: #475569;"            /* Apagamos el texto de ítems residuales si los hubiera */
        "}";



    this->setStyleSheet(qss);
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

    // 1. Crear el contenedor del diálogo nativo
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Configurar Parámetros de Cálculo"));
    dlg.resize(460, 240);

    QVBoxLayout *layoutPrincipal = new QVBoxLayout(&dlg);
    layoutPrincipal->setContentsMargins(20, 20, 20, 20);
    layoutPrincipal->setSpacing(14);

    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(10);

    // Combo A: Método / Hamiltoniano
    QLabel *lblMetodo = new QLabel(tr("Método Químico:"), &dlg);
    QComboBox *comboMetodo = new QComboBox(&dlg);
    comboMetodo->addItems({"PM7", "PM6", "AM1", "RM1", "MNDO"});

    // Combo B: Tipo de Tarea de simulación
    QLabel *lblTarea = new QLabel(tr("Tipo de Tarea:"), &dlg);
    QComboBox *comboTarea = new QComboBox(&dlg);
    comboTarea->addItem(tr("1.- Optimización Geométrica Rápida"), "OPT");
    comboTarea->addItem(tr("2.- Trayectoria Completa (Cálculo Dinámico)"), "OPT AUX LARGE");
    comboTarea->addItem(tr("3.- Modos Vibracionales / Frecuencias IR"), "AUX OPT FORCE");

    // Campo C: Línea libre para keywords personalizadas
    QLabel *lblExtras = new QLabel(tr("Keywords Adicionales:"), &dlg);
    QLineEdit *txtExtras = new QLineEdit(&dlg);
    txtExtras->setPlaceholderText(tr("Ej: CHARGE=1 PRECISE EPS=78.4"));

    grid->addWidget(lblMetodo, 0, 0);   grid->addWidget(comboMetodo, 0, 1);
    grid->addWidget(lblTarea, 1, 0);    grid->addWidget(comboTarea, 1, 1);
    grid->addWidget(lblExtras, 2, 0);   grid->addWidget(txtExtras, 2, 1);

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
    // 🔍 FASE DE APERTURA: Cargar el estado actual del View en los controles
    // =================================================================================
    QString argsActuales = m_view->getArgumentos().toUpper();

    // Sincronizar Combo Método
    for(int i = 0; i < comboMetodo->count(); ++i) {
        if(argsActuales.contains(comboMetodo->itemText(i))) {
            comboMetodo->setCurrentIndex(i);
            break;
        }
    }

    // Sincronizar Combo Tarea analizando prioridades
    if(argsActuales.contains("FORCE")) {
        comboTarea->setCurrentIndex(2); // AUX FORCE
    } else if(argsActuales.contains("LARGE") || argsActuales.contains("AUX")) {
        comboTarea->setCurrentIndex(1); // OPT AUX LARGE
    } else {
        comboTarea->setCurrentIndex(0); // OPT
    }

    // Aislar ÚNICAMENTE las palabras libres del usuario (Limpieza por tokens exactos)
    QStringList tokensActuales = argsActuales.split(' ', Qt::SkipEmptyParts);
    QStringList keywordsEstructurales = {"PM7", "PM6", "AM1", "RM1", "MNDO", "OPT", "FORCE", "AUX", "LARGE", "DIPOLE", "NOINTER"};
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

        // 2. Inyectar los comandos de la tarea del combo
        QString tareaSeleccionada = comboTarea->currentData().toString();
        nuevosKeywords << tareaSeleccionada.split(' ', Qt::SkipEmptyParts);

        // 3. Inyectar las keywords manuales ingresadas en el cuadro de texto
        QString extrasTexto = txtExtras->text().trimmed().toUpper();
        if(!extrasTexto.isEmpty()) {
            nuevosKeywords << extrasTexto.split(' ', Qt::SkipEmptyParts);
        }

        // 4. Parámetros estáticos de acople requeridos por la aplicación
        nuevosKeywords << "DIPOLE" << "NOINTER";

        // Generación de la cadena unificada final (sin acumulación previa)
        QString comandoFinal = nuevosKeywords.join(" ").simplified().trimmed();

        // Guardamos el comando en el cerebro del programa y actualizamos el cuadro de texto
        m_view->setArgumentos(comandoFinal);
        ui->etArgumentos->setText(comandoFinal);
    }
}

void Vista::configurarCalculoGaussian(){

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
        configurarCalculoGaussian();
    }
}

void Vista::on_btnLanzar_clicked(){
    m_view->lanzarCalculoQm();
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

