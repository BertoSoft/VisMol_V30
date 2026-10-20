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

};





#endif // LIENZO_H