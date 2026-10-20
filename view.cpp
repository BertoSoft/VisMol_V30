#include "view.h"
#include "atomos.h"

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

void View::setClick(QVector3D posClick){
    switch (m_estado.modoEditor) {
    case ModoSeleccion:
        procesarClickSeleccion(posClick);
        break;
    case ModoDibujo:
        procesarClickDibujo(posClick);
        break;
    case ModoCrearEnlace:
        procesarClickCrearEnlace(posClick);
        break;
    case ModoRotacion3D:
        procesarClickRotacion(posClick);
        break;
    default:
        return;
    }

}

void View::nuevoProyecto(){
    limpiarLienzo();

    m_estado.estaEmpezado   = true;

    // Limpieza profunda del historial al cerrar, limpiar o cambiar de lienzo
    //m_atras.clear();
    //m_adelante.clear();

    notificaEstado();
}

void View::abrirProyecto(){







    m_estado.estaEmpezado = true;
    notificaEstado();
}

void View::cerrarProyecto(){


    m_estado.estaEmpezado   = false;
    notificaEstado();
}

Estado View::getEstado(){
    return m_estado;
}

QString View::getAtomoActivo(){
    return m_atomoActivo;
}

QVector<Atomo> View::getListaAtomos(){
    return m_listaAtomos;
}

QVector<Enlace> View::getLIstaEnlaces(){
    return m_listaEnlaces;
}

void View::setElementoActivo(QString simbolo){
    if(simbolo == "") return;

    if(m_estado.modoEditor != ModoEditor::ModoDibujo){
        m_estado.modoEditor = ModoEditor::ModoDibujo;
    }

    m_atomoActivo = simbolo;

    notificaEstado();
}

void View::setModoEditor(ModoEditor modo){
    if(modo == m_estado.modoEditor) return;

    m_estado.modoEditor = modo;

    notificaEstado();
}

// =========================================================================
// FUNCIONES PRIVADAS
// =========================================================================

void View::resetValores(){

    m_atomoActivo               = "";
    m_contadorId                = 0;
    m_rutaArchivoActual         = "";

    m_estado.modoEditor         = ModoEditor::ModoSeleccion;
    m_estado.estaVacio          = true;
    m_estado.estaGuardado       = false;
    m_estado.estaModificado     = false;
    m_estado.hayProceso         = false;
    m_estado.estaEmpezado       = false;
}

void View::notificaEstado(){

    m_estado.estaVacio   = m_listaAtomos.isEmpty() && m_listaEnlaces.isEmpty();

    emit actualizarLienzo();
    emit actualizarVista(m_estado);
}

void View::limpiarLienzo(){

    resetValores();

    m_estado.estaVacio      = true;
    m_estado.estaGuardado   = false;
    m_estado.estaEmpezado   = true;
    m_estado.estaModificado = false;
    m_estado.modoEditor     = ModoEditor::ModoSeleccion;

    emit actualizarVista(m_estado);
}

void View::procesarClickSeleccion(QVector3D posClick){

}

void View::procesarClickDibujo(QVector3D posClick){

    if(getIdAtomoFromPosicion(posClick) < 0){
        Atomo nuevoAtomo;

        nuevoAtomo.id       = ++m_contadorId;
        nuevoAtomo.simbolo  = m_atomoActivo;
        nuevoAtomo.posicion = posClick;

        m_listaAtomos.append(nuevoAtomo);

        notificaEstado();
    }

}

void View::procesarClickCrearEnlace(QVector3D posClick){

}

void View::procesarClickRotacion(QVector3D posClick){

}

int View::getIdAtomoFromPosicion(QVector3D posClick){

    for(const Atomo& atomo: m_listaAtomos){
        float distancia = posClick.distanceToPoint(atomo.posicion);
        float radio = Atomos::getRadio(atomo.simbolo); // SI no existe devuelve 15.0 A.
        if(distancia < radio){
            return atomo.id;
        }
    }
    return -1;
}