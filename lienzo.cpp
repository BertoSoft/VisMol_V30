#include "lienzo.h"

#include <QMouseEvent>

Lienzo::Lienzo(QObject *parent)
    : QGraphicsScene(parent){
    // Aquí inicializaremos el lienzo molecular más adelante
}

Lienzo::~Lienzo(){
    // Limpieza si fuera necesaria
}

// =========================================================================
// BLOQUE: MÉTODOS DE INICIALIZACIÓN POR CAPAS MODULARES
// =========================================================================
void Lienzo::mousePressEvent(QGraphicsSceneMouseEvent *mouseEv){

}

void Lienzo::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEv){

}

void Lienzo::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEv){

}