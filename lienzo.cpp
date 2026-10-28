#include "lienzo.h"
#include "atomos.h"
#include "mat.h"

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

    // centramos la molecula
    this->centerOn(0.0, 0.0);

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

        // 🎯 SOLUCIÓN 1: Rotamos la posición 3D original del átomo bajo los ángulos de la cámara
        QPointF posRotada = Mat::rotarPunto(atomo.posicion, m_anguloX, m_anguloY);

        // 🎯 SOLUCIÓN 2: Aplicamos la traslación visual en tiempo real acumulada por el arrastre
        float xCentro = posRotada.x() + m_deltaX;
        float yCentro = posRotada.y() + m_deltaY;

        // En QGraphicsScene las elipses se dibujan desde su esquina superior izquierda
        float xEsquina = (xCentro - radio);
        float yEsquina = (yCentro - radio);

        // Elipse del Átomo
        QGraphicsEllipseItem *elipse = m_escena->addEllipse(xEsquina, yEsquina, diametro, diametro);

        // Configuración de estilos estéticos y colores
        elipse->setBrush(QBrush(color));
        elipse->setPen(QPen(color.darker(150), 1.5));

        elipse->setZValue(1);

        // Círculo de Átomo Seleccionado (Anillo azul)
        if (m_view && m_view->getIdAtomoSeleccionado() == atomo.id) {
            qreal radioAnillo = radio + 6.0;
            qreal xAnillo = xCentro - radioAnillo;
            qreal yAnillo = yCentro - radioAnillo;
            QGraphicsEllipseItem* anilloSel = m_escena->addEllipse(xAnillo, yAnillo, radioAnillo * 2.0, radioAnillo * 2.0);
            QPen penSel(QColor("#38BDF8")); penSel.setWidthF(2.5f); penSel.setStyle(Qt::DashLine);
            anilloSel->setPen(penSel); anilloSel->setZValue(3);
        }

        // 2. 📐 NUEVO BLOQUE: Halos estroboscópicos multivariable exclusivos para el MODO MEDIDAS
        if (m_view && m_view->getEstado().modoEditor == ModoMedida) {
            QColor colorAnilloMedida;
            bool pintarAnillo = false;

            if (atomo.id == m_view->getIdAtomoMedida1()) {
                colorAnilloMedida = QColor("#FACC15"); // 🟡 Amarillo para el Átomo 1 (Punto de partida)
                pintarAnillo = true;
            }
            else if (atomo.id == m_view->getIdAtomoMedida2()) {
                colorAnilloMedida = QColor("#FB923C"); // 🟠 Naranja para el Átomo 2 (Distancia establecida)
                pintarAnillo = true;
            }
            else if (atomo.id == m_view->getIdAtomoMedida3()) {
                colorAnilloMedida = QColor("#E879F9"); // 🟣 Magenta para el Átomo 3 (Ángulo definido)
                pintarAnillo = true;
            }
            else if (atomo.id == m_view->getIdAtomoMedida4()) {
                colorAnilloMedida = QColor("#22C55E"); // 🟢 Verde esmeralda para el Átomo 4 (Cierre del Diedro)
                pintarAnillo = true;
            }

            if (pintarAnillo) {
                qreal radioAnillo = radio + 5.0;
                qreal xAnillo = xCentro - radioAnillo;
                qreal yAnillo = yCentro - radioAnillo;
                QGraphicsEllipseItem* anilloMedida = m_escena->addEllipse(xAnillo, yAnillo, radioAnillo * 2.0, radioAnillo * 2.0);

                QPen penMedida(colorAnilloMedida);
                penMedida.setWidthF(2.5f);
                penMedida.setStyle(Qt::SolidLine); // Línea continua para que destaque con fuerza

                anilloMedida->setPen(penMedida);
                anilloMedida->setZValue(3);
            }
        }

        // =========================================================================
        // 🎯 CAMBIO QUIRÚRGICO: Concatenamos el Símbolo Químico con su ID numérico
        // =========================================================================
        QString textoAtomoConId = QString("%1(%2)").arg(atomo.simbolo).arg(atomo.id);        QGraphicsTextItem* texto = new QGraphicsTextItem(textoAtomoConId);

        QFont fuente("Segoe UI", 10, QFont::Bold);
        texto->setFont(fuente);

        // Si el átomo es Hidrógeno (Blanco), pintamos el texto oscuro; si no, blanco para contrastar
        texto->setDefaultTextColor(atomo.simbolo == "H" ? QColor("#0F172A") : Qt::white);

        // Centrado matemático exacto del bloque de texto expandido dentro de la esfera desplazada
        QRectF limitesTexto = texto->boundingRect();
        texto->setPos(xCentro - (limitesTexto.width() / 2.0f),
                      yCentro - (limitesTexto.height() / 2.0f));

        texto->setZValue(2);
        m_escena->addItem(texto);
    }
}

void Lienzo::renderEnlaces(){
    if(!m_view) return;

    QVector<Enlace> listaEnlaces    = m_view->getListaEnlaces();
    QVector<Atomo>  listaAtomos     = m_view->getListaAtomos();

    QPen penEnlaces(QColor("#D1D5DB"));
    penEnlaces.setWidthF(3.0f);
    penEnlaces.setCapStyle(Qt::RoundCap);
    const qreal offsetDist = 8.0;

    for(const Enlace& enlace: listaEnlaces){
        QVector3D   posOrigen;
        QVector3D   posDestino;
        bool        existeOrigen    = false;
        bool        existeDestino   = false;

        for(const Atomo& atomo: listaAtomos){
            if(atomo.id == enlace.id_atomo1){
                posOrigen       = atomo.posicion;
                existeOrigen    = true;
            }
            if(atomo.id == enlace.id_atomo2){
                posDestino      = atomo.posicion;
                existeDestino   = true;
            }
            if(existeOrigen && existeDestino) break;
        }

        if(existeOrigen && existeDestino){
            QPointF p1, p2;

            // Rotamos las posiciones base bajo los mismos ángulos de cámara
            p1 = Mat::rotarPunto(posOrigen, m_anguloX, m_anguloY);
            p2 = Mat::rotarPunto(posDestino, m_anguloX, m_anguloY);

            // 🎯 SOLUCIÓN 2: Sumamos la traslación visual interactiva (m_deltaX / m_deltaY)
            p1.setX(p1.x() + m_deltaX);
            p1.setY(p1.y() + m_deltaY);
            p2.setX(p2.x() + m_deltaX);
            p2.setY(p2.y() + m_deltaY);

            qreal dx = p2.x() - p1.x();
            qreal dy = p2.y() - p1.y();
            qreal longitud = qSqrt(dx * dx + dy * dy);
            if (longitud < 0.001) continue;
            qreal px = -dy / longitud;
            qreal py = dx / longitud;

            if (enlace.orden == 1) {
                m_escena->addLine(p1.x(), p1.y(), p2.x(), p2.y(), penEnlaces)->setZValue(0);
            }
            else if (enlace.orden == 2) {
                qreal ox = px * (offsetDist / 2.0); qreal oy = py * (offsetDist / 2.0);
                m_escena->addLine(p1.x() + ox, p1.y() + oy, p2.x() + ox, p2.y() + oy, penEnlaces)->setZValue(0);
                m_escena->addLine(p1.x() - ox, p1.y() - oy, p2.x() - ox, p2.y() - oy, penEnlaces)->setZValue(0);
            }
            else if (enlace.orden >= 3) {
                qreal ox = px * offsetDist; qreal oy = py * offsetDist;
                m_escena->addLine(p1.x(), p1.y(), p2.x(), p2.y(), penEnlaces)->setZValue(0);
                m_escena->addLine(p1.x() + ox, p1.y() + oy, p2.x() + ox, p2.y() + oy, penEnlaces)->setZValue(0);
                m_escena->addLine(p1.x() - ox, p1.y() - oy, p2.x() - ox, p2.y() - oy, penEnlaces)->setZValue(0);
            }
        }
    }

    // =========================================================================
    // 📐 NUEVO BLOQUE: LÍNEAS DE TRAZOS (PUNTOS) PARA EL MODO MEDIDAS
    // =========================================================================
    if (m_view && m_view->getEstado().modoEditor == ModoEditor::ModoMedida) {

        // Configuración de un pincel estético amarillo fosforescente y discontinuo
        QPen penMedidas(QColor("#FACC15"));
        penMedidas.setWidthF(2.0f);
        penMedidas.setStyle(Qt::DotLine); // ◄ Línea de puntos reglamentaria

        // Recuperamos los IDs activos en el buffer del cerebro
        int id1 = m_view->getIdAtomoMedida1();
        int id2 = m_view->getIdAtomoMedida2();
        int id3 = m_view->getIdAtomoMedida3();
        int id4 = m_view->getIdAtomoMedida4(); // ◄ Recuperamos el nuevo ID

        QPointF p1, p2, p3, p4;
        bool tieneP1 = false, tieneP2 = false, tieneP3 = false, tieneP4 = false;

        for (const Atomo& atomo : listaAtomos) {
            if (atomo.id == id1) {
                p1 = Mat::rotarPunto(atomo.posicion, m_anguloX, m_anguloY);
                p1.setX(p1.x() + m_deltaX); p1.setY(p1.y() + m_deltaY);
                tieneP1 = true;
            }
            if (atomo.id == id2) {
                p2 = Mat::rotarPunto(atomo.posicion, m_anguloX, m_anguloY);
                p2.setX(p2.x() + m_deltaX); p2.setY(p2.y() + m_deltaY);
                tieneP2 = true;
            }
            if (atomo.id == id3) {
                p3 = Mat::rotarPunto(atomo.posicion, m_anguloX, m_anguloY);
                p3.setX(p3.x() + m_deltaX); p3.setY(p3.y() + m_deltaY);
                tieneP3 = true;
            }
            if (atomo.id == id4) {
                p4 = Mat::rotarPunto(atomo.posicion, m_anguloX, m_anguloY);
                p4.setX(p4.x() + m_deltaX); p4.setY(p4.y() + m_deltaY);
                tieneP4 = true;
            }
        }

        // Tramo A: segmento de la distancia (Átomo 1 a Átomo 2)
        if (tieneP1 && tieneP2) {
            m_escena->addLine(p1.x(), p1.y(), p2.x(), p2.y(), penMedidas)->setZValue(3);
        }

        // Tramo B: ala del ángulo valencial (Átomo 2 a Átomo 3)
        if (tieneP2 && tieneP3) {
            m_escena->addLine(p2.x(), p2.y(), p3.x(), p3.y(), penMedidas)->setZValue(3);
        }

        // 🎯 Tramo C: El eslabón del Diedro (Conecta Átomo 3 con Átomo 4)
        if (tieneP3 && tieneP4) {
            m_escena->addLine(p3.x(), p3.y(), p4.x(), p4.y(), penMedidas)->setZValue(3);
        }
    }
}

// =========================================================================
// BLOQUE: MÉTODOS PROTEGIDOS DE INTERACCION CON EL RATON
// =========================================================================
void Lienzo::mousePressEvent(QMouseEvent *mouseEv){
    if(!m_view) return;

    m_ultimaPosRat = mouseEv->pos();

    // Si no
    if(m_view->getEstado().modoEditor != ModoRotacion3D) {
        // Capturamos el punto exacto en píxeles puros de la escena
        QPointF posClick = this->mapToScene(mouseEv->pos());

        // ─── ELIMINADA LA DIVISIÓN: Mandamos píxeles puros directamente al Cerebro ───
        if(mouseEv->button() == Qt::LeftButton){
            m_view->setClick(QVector3D(posClick.x(), posClick.y(), 0.0f));
        }
        if(mouseEv->button() == Qt::RightButton){
            m_view->setClickDerecho(QVector3D(posClick.x(), posClick.y(), 0.0f));
        }
    }

    QGraphicsView::mousePressEvent(mouseEv);
}

void Lienzo::mouseMoveEvent(QMouseEvent *mouseEv){
    if (!m_view) {
        QGraphicsView::mouseMoveEvent(mouseEv);
        return;
    }

    // Calculamos el micro-delta instantáneo entre la posición intermedia anterior y la actual
    int deltaX = mouseEv->pos().x() - m_ultimaPosRat.x();
    int deltaY = mouseEv->pos().y() - m_ultimaPosRat.y();

    // ─── POLÍTICA A: MODO ROTACIÓN 3D (MUTAR ÁNGULOS LOCALES) ───
    if(m_view->getEstado().modoEditor == ModoRotacion3D && (mouseEv->buttons() & Qt::LeftButton)) {
        m_anguloY += deltaX * 0.5f;
        m_anguloX += deltaY * 0.5f;
        m_anguloX = qMax(-85.0f, qMin(85.0f, m_anguloX));

        m_ultimaPosRat = mouseEv->pos(); // Sincronizamos el ancla intermedia
        renderLienzo();
    }
    // ─── POLÍTICA B: MODO SELECCIÓN (MUTAR DELTAS LOCALES, EL CEREBRO NO TRABAJA) ───
    else if(m_view->getEstado().modoEditor == ModoSeleccion && (mouseEv->buttons() & Qt::LeftButton)) {
        if (deltaX != 0 || deltaY != 0) {
            // Refrescamos y acumulamos los deltas locales en el lienzo
            m_deltaX += static_cast<qreal>(deltaX);
            m_deltaY += static_cast<qreal>(deltaY);

            m_ultimaPosRat = mouseEv->pos(); // Sincronizamos el ancla intermedia exactamente igual que arriba
            renderLienzo();             // Forzamos el repintado óptico fluido en tiempo real
        }
    }
    else {
        QGraphicsView::mouseMoveEvent(mouseEv);
    }
}

void Lienzo::mouseReleaseEvent(QMouseEvent *mouseEv){
    if(!m_view) {
        QGraphicsView::mouseReleaseEvent(mouseEv);
        return;
    }

    // ─── TRANSACCIÓN DE CIERRE AL LEVANTAR EL BOTÓN IZQUIERDO EN SELECCIÓN ───
    if(m_view->getEstado().modoEditor == ModoSeleccion && mouseEv->button() == Qt::LeftButton) {
        // 1. Convertimos los píxeles de pantalla a coordenadas finales reales de la escena
        QPointF posClickEscenaFinal = this->mapToScene(mouseEv->pos());

        // 3. LIMPIEZA PROTOCOLARIA: Reseteamos los acumuladores visuales a cero de golpe
        m_deltaX = 0.0;
        m_deltaY = 0.0;
        m_ultimaPosRat = QPoint(0, 0); // Limpiamos la caché del ancla intermedia de Qt

        // 4. ENVIAR AL CEREBRO: Mandamos la coordenada final pura de la escena
        // El cerebro computará la diferencia real y el lienzo queda limpio para la próxima acción
        m_view->setClick(QVector3D(posClickEscenaFinal.x(), posClickEscenaFinal.y(), 0.0f));

        mouseEv->accept();
        return;
    }

    // ─── 🎯 NUEVO: TRANSACCIÓN DE CIERRE EN MODO ROTACIÓN 3D (MOVIMIENTO SÓLO ÓPTICO) ───
    if(m_view->getEstado().modoEditor == ModoRotacion3D && mouseEv->button() == Qt::LeftButton) {
        // Reseteamos por completo la orientación de la cámara virtual a su plano basal
        m_anguloX = 0.0f;
        m_anguloY = 0.0f;
        m_ultimaPosRat = QPoint(0, 0);

        // Forzamos el redibujado síncrono inmediato para devolver la molécula a su estado original
        renderLienzo();

        mouseEv->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(mouseEv);
}


