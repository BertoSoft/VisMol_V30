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

    // 🎯 BANDERAS Y CONTENEDORES PARA EL ESPECTRO IR
    bool            m_hayFrecuencias = false;
    QVector<float>  m_frecuenciasX;
    QVector<float>  m_intensidadesY;
    double          m_minX_Espectro = 0.0;
    double          m_maxX_Espectro = 0.0;
    QVector<int>    m_numerosModoReal;

    int             m_idxModoActivoGrafica = -1; // Guardará el índice activo (-1 si ninguno)

    int             m_pasoActual;
    double          m_minY;
    double          m_maxY;

public:
    explicit Grafica(QWidget *parent = nullptr);
    virtual ~Grafica() = default;

    void setDatos(const QVector<Trayectoria>& pelicula);
    void setDatosCalores(const QVector<float>& calores);
    void setDatosEspectro(const QVector<ModoVibracional>& modos);

    void setPasoActual(int paso);
    void setModoActivo(int idxModo);

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // GRAFICA_H