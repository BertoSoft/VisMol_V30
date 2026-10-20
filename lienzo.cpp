#include "lienzo.h"
#include "atomos.h"

#include <QMouseEvent>
#include <QObject>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>


// =========================================================================
// CLASE PRINCIPAL: LIENZO, ENCARGADA DEL DIBUJO EN VISOR MOLECULA
// =========================================================================
Lienzo::Lienzo(QWidget* parent)
    : QGraphicsView(parent){
    // Aquí inicializaremos el lienzo molecular más adelante
    m_escena = new QGraphicsScene(this);

    // Fijamos el tamaño del espacio de trabajo en Amstrongs
    m_escena->setSceneRect(-5000, -5000, 10000, 10000);
    this->setScene(m_escena);

    // Opciones estéticas profesionales para un renderizado de átomos nítido
    this->setRenderHint(QPainter::Antialiasing);

    // Evita que aparezcan barras de scroll molestas
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);


}

Lienzo::~Lienzo(){
    // Limpieza si fuera necesaria
}


// =========================================================================
// BLOQUE: MÉTODOS PUBLICOS
// =========================================================================
void Lienzo::initView(View* view){
    m_view = view;
}

void Lienzo::renderLienzo(){

    m_escena->clear();

    // Dibujamos los atomos
    renderAtomos();

    // Dibujamos los enlaces
    renderEnlaces();

}

// =========================================================================
// BLOQUE: MÉTODOS PRIVADOS
// =========================================================================
void Lienzo::renderAtomos(){
    if(!m_view) return;

    QVector<Atomo> listaAtomos = m_view->getListaAtomos();

    for(const Atomo& atomo: listaAtomos){
        float   radio       = Atomos::getRadio(atomo.simbolo);
        QColor  color       = Atomos::getColor(atomo.simbolo);
        float   diametro    = radio * 2.0f;

        // 🎯 En QGraphicsScene, las elipses se dibujan desde su esquina superior izquierda.
        // Restamos el radio a las coordenadas X e Y para que la esfera quede perfectamente centrada.
        float xEsquina = (atomo.posicion.x() - radio);
        float yEsquina = (atomo.posicion.y() - radio);

        // Elipse del Atomo
        QGraphicsEllipseItem *elipse = m_escena->addEllipse(xEsquina, yEsquina, diametro, diametro);

        // Configuración de estilos estéticos y colores
        elipse->setBrush(QBrush(color)); // Relleno de color del elemento
        elipse->setPen(QPen(color.darker(150), 1.5)); // Borde un poco más oscuro para dar relieve

        m_escena->addItem(elipse);

        // TExto del Atomo
        QGraphicsTextItem* texto = new QGraphicsTextItem(atomo.simbolo);

        // Estilo de la fuente tipográfica tipométrica
        QFont fuente("Segoe UI", 10, QFont::Bold);
        texto->setFont(fuente);

        // Si el átomo es Hidrógeno (Blanco), pintamos el texto oscuro; si no, texto blanco para contrastar
        texto->setDefaultTextColor(atomo.simbolo == "H" ? QColor("#0F172A") : Qt::white);

        // Centrado matemático exacto del bloque de texto dentro de la esfera
        QRectF limitesTexto = texto->boundingRect();
        texto->setPos(atomo.posicion.x() - (limitesTexto.width() / 2.0f),
                      atomo.posicion.y() - (limitesTexto.height() / 2.0f));

        m_escena->addItem(texto);
    }
}

void Lienzo::renderEnlaces(){

}



// =========================================================================
// BLOQUE: MÉTODOS PROTEGIDOS DE INTERACCION CON EL RATON
// =========================================================================
void Lienzo::mousePressEvent(QMouseEvent *mouseEv){
    if(!m_view) return;

    if(m_view->getEstado().modoEditor != ModoEditor::ModoRotacion3D){
        QPointF puntoEscena = this->mapToScene(mouseEv->pos());

        QVector3D pos3D = QVector3D(puntoEscena.x(), puntoEscena.y(), 0.0f);

        if(mouseEv->button() == Qt::LeftButton){
            m_view->setClick(pos3D);
        }

        if(mouseEv->button() == Qt::RightButton){

        }

    }

}

void Lienzo::mouseMoveEvent(QMouseEvent *mouseEv){

}

void Lienzo::mouseReleaseEvent(QMouseEvent *mouseEv){

}


