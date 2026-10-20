#include "vista.h"
#include "ui_vista.h"

#include <QMessageBox>
#include <QDate>
#include <QTime>
#include <QTimer>

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
    ui->barraDerecha->setEnabled(!estado.estaVacio);
    if(!estado.estaVacio){
        ui->btnLanzar->setEnabled(!estado.hayProceso);
        ui->btnAbortar->setEnabled(estado.hayProceso);

        ui->frameEjecucion->setEnabled(!estado.hayProceso);
        ui->frameSoftware->setEnabled(!estado.hayProceso);
    }

    // 6.- Barra estado
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
            // Usamos el dato inyectado en el estado
            txt = tr(" Modo Dibujo 🧪 | Añadiendo elemento: %1 (Clic en lienzo para colocar)").arg(m_view->getAtomoActivo());
            break;
        }
        m_lblMesaje->setText(txt);
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

    // Si hay peligro, abrimos el cuadro de diálogo (responsabilidad de la Vista)
    QMessageBox::StandardButton respuesta = QMessageBox::warning(
        this,
        tr("Cambios sin guardar"),
        tr("El proyecto actual tiene cambios sin guardar.\n¿Desea guardarlos antes de continuar?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );

    if (respuesta == QMessageBox::Save) {
        on_actionGuardar_triggered();
        return true; // Continuamos tras guardar
    }
    else if (respuesta == QMessageBox::Discard) {
        return true; // Continuamos descartando los cambios
    }

    return false; // El usuario pulsó Cancelar, detenemos cualquier acción
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
    m_view->cerrarProyecto();
}

void Vista::on_actionAbrir_triggered(){
    m_view->abrirProyecto();
}

void Vista::on_actionGuardar_triggered(){

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

