#include "vista.h"
#include "ui_vista.h"

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

void Vista::initUi(){
    initGrupoElementos();
    setEstilos();
}

void Vista::initView(View* view){
    m_view = view;
}

void Vista::initLienzo(Lienzo* lienzo){
    m_lienzo = lienzo;
    initEscena();
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

void Vista::initEscena(){

    ui->visorMolecular->setScene(m_lienzo);

    //Activamos trucos de suavizado gráfico para que los átomos redondos no se vean pixelados
    ui->visorMolecular->setRenderHint(QPainter::Antialiasing);
    ui->visorMolecular->setRenderHint(QPainter::SmoothPixmapTransform);
}

//======================================================================================
// Funciones Privadas
//======================================================================================
void Vista::actualizarUi(Estado estado){

    // 1.- Botones del menu Archivo
    ui->actionCerrar->setEnabled(estado.estaEmpezado);
    ui->actionGuardar->setEnabled(!estado.estaVacio);
    ui->actionGuardarComo->setEnabled(!estado.estaVacio);

    // 2.- Botones del menu Edicion
    ui->actionDeshacer->setEnabled(false);
    ui->actionRehacer->setEnabled(false);
    ui->actionComentarioProyecto->setEnabled(!estado.estaVacio);
    ui->actionLimpiarLienzo->setEnabled(!estado.estaVacio);

    // 3.- Botones de Calculo
    ui->actionConfigurarLanzar->setEnabled(estado.estaEmpezado);
    ui->actionDetenerSimulacion->setEnabled(estado.hayProceso);

    // 4.- barra elementos
    if(estado.estaEmpezado){
        ui->btnSeleccion->setChecked(estado.modoEditor == ModoEditor::ModoSeleccion);
        ui->btnModoEnlace->setChecked(estado.modoEditor == ModoEditor::ModoCrearEnlace);
        ui->btnModoRotar->setChecked(estado.modoEditor == ModoEditor::ModoRotacion3D);
        ui->barraIzda->setEnabled(estado.estaEmpezado);
    }
    else{
        m_grupoIzdo->setExclusive(false);
        if(m_grupoIzdo->checkedButton()){
            m_grupoIzdo->checkedButton()->setChecked(false);
        }
        m_grupoIzdo->setExclusive(true);
        ui->barraIzda->setEnabled(false);
    }

    // 5.- Barra derecha
    ui->barraDerecha->setEnabled(estado.estaEmpezado);
    if(estado.estaEmpezado){
        ui->btnLanzar->setEnabled(!estado.estaVacio && !estado.hayProceso);
        ui->btnAbortar->setEnabled(estado.hayProceso);

        ui->frameEjecucion->setEnabled(!estado.hayProceso);
        ui->frameSoftware->setEnabled(!estado.hayProceso);
    }

}

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

///======================================================================================
// SLOTS PRIVADOS de Botones
//======================================================================================
void Vista::on_actionSalir_triggered(){
    close(); // Cierra limpiamente la ventana principal y finaliza el proceso
}

void Vista::on_actionNuevo_triggered(){
    m_view->nuevoProyecto();
}

void Vista::on_actionCerrar_triggered(){
    m_view->cerrarProyecto();
}

void Vista::on_actionAbrir_triggered(){
    m_view->abrirProyecto();
}

