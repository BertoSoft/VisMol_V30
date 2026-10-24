#ifndef GRAFICA_H
#define GRAFICA_H

#include <QWidget>
#include <QVector>
#include <QPaintEvent>
#include "view.h" // Necesario para reconocer la estructura Trayectoria

class Grafica : public QWidget {
    Q_OBJECT
private:
    QVector<double> m_valoresEnergia;
    int m_pasoActual;
    double m_minY;
    double m_maxY;

public:
    explicit Grafica(QWidget *parent = nullptr);
    virtual ~Grafica() = default;

    void setDatos(const QVector<Trayectoria>& pelicula);
    void setDatosCalores(const QVector<float>& calores);

    void setPasoActual(int paso);

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // GRAFICA_H