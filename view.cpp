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

void View::nuevoProyecto(){


    m_estado.estaEmpezado = true;
    emit actualizarVista(m_estado);
}

void View::abrirProyecto(){







    m_estado.estaEmpezado = true;
    emit actualizarVista(m_estado);
}

void View::cerrarProyecto(){


    m_estado.estaEmpezado   = false;
    m_estado.estaVacio      = true;
    m_estado.estaModificado = false;

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
    m_estado.estaEmpezado       = false;
}