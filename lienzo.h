#ifndef LIENZO_H
#define LIENZO_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent> // Añadido para los eventos del ratón
#include <QObject>
#include <QVector3D>


// =========================================================================
// CLASE PRINCIPAL: LIENZO, ENCARGADA DEL DIBUJO EN VISOR MOLECULA
// =========================================================================

class Lienzo : public QGraphicsScene {
    Q_OBJECT

public:
    explicit Lienzo(QObject *parent = nullptr);
    ~Lienzo() override;

protected:
// =========================================================================
// BLOQUE: MÉTODOS PROTEGIDOS DE INTERACCION CON EL RATON
// =========================================================================
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
};

#endif // LIENZO_H