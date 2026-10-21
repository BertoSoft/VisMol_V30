#ifndef LIENZO_H
#define LIENZO_H

#include "view.h"


#include <QGraphicsScene>
#include <QGraphicsView>
#include <QObject>
#include <QVector3D>


// =========================================================================
// CLASE PRINCIPAL: LIENZO, ENCARGADA DEL DIBUJO EN VISOR MOLECULA
// =========================================================================

class Lienzo : public QGraphicsView {
    Q_OBJECT

public:
    explicit Lienzo(QWidget *parent = nullptr);
    ~Lienzo() override;


// =========================================================================
// BLOQUE: MÉTODOS PUBLICOS
// =========================================================================

    void initView(View* view);
    void renderLienzo();


// =========================================================================
// BLOQUE: MÉTODOS PROTEGIDOS DE INTERACCION CON EL RATON
// =========================================================================
protected:

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

// =========================================================================
// BLOQUE: MÉTODOS PRIVADOS
// =========================================================================
private:

    void renderAtomos();
    void renderEnlaces();

    QGraphicsScene*     m_escena    = nullptr;
    View*               m_view      = nullptr;

    // =========================================================================
    // CONTROL DE ÁNGULOS Y MEMORIA DEL RATÓN PARA ROTACIÓN 3D
    // =========================================================================
    float   m_anguloX      = 0.0f;  // Rotación vertical (Pitch)
    float   m_anguloY      = 0.0f;  // Rotación horizontal (Yaw)
    float   m_deltaX       = 0.0f;
    float   m_deltaY       = 0.0f;
    QPoint  m_ultimaPosRat;         // Guarda el último píxel del cursor para calcular el movimiento (Delta)

};





#endif // LIENZO_H