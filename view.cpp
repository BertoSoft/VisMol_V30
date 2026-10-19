#include "view.h"

View::View(QObject *parent)
    : QObject(parent){

}

View::~View(){

}


// =========================================================================
// FUNCIONES PUBLICAS
// =========================================================================
void View::initUi(){
    resetValores();
    emit actualizarVista(m_estado);
}

// =========================================================================
// FUNCIONES PRIVADAS
// =========================================================================

void View::resetValores(){

    m_estado.modoEditor         = ModoEditor::ModoSeleccion;
    m_estado.estaVacio          = true;
    m_estado.estaGuardado       = false;
    m_estado.estaModificado     = false;
    m_estado.hayProceso         = false;
}