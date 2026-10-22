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

//======================================================================================
// Funciones SLOT Publicos
//======================================================================================
void Vista::actualizarUi(Estado estado){


    // 0.- Control Dinámico del Título (Aviso de Guardado con NOMBRE_APP primero)
    if (!estado.estaEmpezado) {
        this->setWindowTitle(QString("%1 - Sin Proyecto").arg(NOMBRE_APP));
    }
    else {
        // Determinamos el nombre del archivo. Si m_rutaArchivoActual está vacía, es un "Proyecto Nuevo"
        QString nombreArchivo = tr("Proyecto Nuevo");
        if (m_view && !m_view->getRutaActual().isEmpty()) {
            nombreArchivo = QFileInfo(m_view->getRutaActual()).fileName();
        }

        // Estructura solicitada: NOMBRE_APP -> Archivo
        QString titulo = QString("%1 | %2").arg(NOMBRE_APP).arg(nombreArchivo);

        // Si el estado detecta modificaciones o datos sin salvar, inyectamos el asterisco de aviso
        if (estado.estaModificado) {
            titulo += " *";
        }
        this->setWindowTitle(titulo);
    }

    // 1.- Estado base del Visor Molecular y la Barra Superior
    ui->visorMolecular->setEnabled(estado.estaEmpezado);

    // 2. Bloque de seguridad: Forzar que las acciones de inicio estén SIEMPRE usables en barra y menús
    ui->actionNuevo->setEnabled(true);
    ui->actionAbrir->setEnabled(true);
    ui->actionSalir->setEnabled(true);

    // 3.- Botones del menú y barra de Archivo
    ui->actionCerrar->setEnabled(estado.estaEmpezado);
    ui->actionGuardar->setEnabled(estado.estaEmpezado && !estado.estaVacio);
    ui->actionGuardarComo->setEnabled(estado.estaEmpezado && !estado.estaVacio);

    // 4.- Botones del menú y barra de Edición
    ui->actionDeshacer->setEnabled(estado.hayDeshacer);
    ui->actionRehacer->setEnabled(estado.hayRehacer);
    ui->actionComentarioProyecto->setEnabled(estado.estaEmpezado && !estado.estaVacio);
    ui->actionLimpiarLienzo->setEnabled(estado.estaEmpezado && !estado.estaVacio);

    // 5.- Botones de Cálculo QM (Menú superior)
    ui->actionConfigurarLanzar->setEnabled(estado.estaEmpezado);
    ui->actionDetenerSimulacion->setEnabled(estado.estaEmpezado && estado.hayProceso);

    // 6.- Barra lateral izquierda (Herramientas de edición y Elementos)
    if(estado.estaEmpezado){
        ui->barraIzda->setEnabled(true);
        ui->btnSeleccion->setChecked(estado.modoEditor == ModoEditor::ModoSeleccion);
        ui->btnModoEnlace->setChecked(estado.modoEditor == ModoEditor::ModoCrearEnlace);
        ui->btnModoRotar->setChecked(estado.modoEditor == ModoEditor::ModoRotacion3D);
    }
    else{
        ui->barraIzda->setEnabled(false);
        m_grupoIzdo->setExclusive(false);
        if(m_grupoIzdo->checkedButton()){
            m_grupoIzdo->checkedButton()->setChecked(false);
        }
        m_grupoIzdo->setExclusive(true);
    }

    // 7.- Barra lateral derecha (Configuración cuántica y Control)
    // Solo se activa si el proyecto está empezado Y además tiene átomos en pantalla
    bool usarPanelDerecho = estado.estaEmpezado && !estado.estaVacio;
    ui->barraDerecha->setEnabled(usarPanelDerecho);

    if(usarPanelDerecho){
        ui->btnLanzar->setEnabled(!estado.hayProceso);
        ui->btnAbortar->setEnabled(estado.hayProceso);
        ui->frameEjecucion->setEnabled(!estado.hayProceso);
        ui->frameSoftware->setEnabled(!estado.hayProceso);
    }

    // 8.- Actualización de la Barra de Estado inferior
    if(!estado.estaEmpezado){
        m_lblMesaje->setText(tr(" Listo. Cree un nuevo proyecto o abra un archivo para comenzar."));
    }
    else{
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
        "QTextEdit#txtMiniLog, QListWidget#listaFrecuencias {"
        "    background-color: #030712;"
        "    color: #10B981;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    border: 1px solid #1F2937;"
        "    border-radius: 4px;"
        "}"
        "QFrame#barraDerecha QLabel {"
        "    color: #94A3B8;" // Gris claro elegante para títulos y etiquetas
        "    font-weight: bold;"
        "}"
        "QTextEdit {"
        "    color: #10B981;" // Forzamos que cualquier texto insertado sea verde terminal
        "}"
        "QListWidget::item {"
        "    color: #E2E8F0;" // Texto de la lista de frecuencias en gris blanco
        "}"
        "QListWidget::item:selected {"
        "    background-color: #38BDF8;"
        "    color: #0F172A;"
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

void Vista::on_btnOptRapida_clicked(){
    if (!m_view) return;

    if (sePuedeCerrar()) {
        m_view->optGeometrica();
    }
}

