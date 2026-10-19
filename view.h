#ifndef VIEW_H
#define VIEW_H

#include <QObject>
#include <QVector3D>

// =========================================================================
// ESTRUCTURAS DE DATOS ORIGINALES
// =========================================================================
enum TipoFormato {
    VISMOL,
    DAT,
    OUT,
    ARC,
    AUX,
    FORCE_OUT,
    FORCE_AUX,
    NO_SE_SABE
};

struct Atomo {
    int id;
    QString simbolo;
    QVector3D posicion;
};

struct Enlace {
    int id_atomo1;
    int id_atomo2;
    int orden; // 1 = Simple, 2 = Doble, 3 = Triple
};

struct Trayectoria {
    QVector<Atomo>  listaAtomos;
    QVector<Enlace> listaEnlaces;
};

// Representa el vector de desplazamiento cartesiano para un átomo en una vibración específica
struct Vibraciones {
    int id_atomo;      // Enlace con el átomo correspondiente
    float dx;          // Desplazamiento en el eje X
    float dy;          // Desplazamiento en el eje Y
    float dz;          // Desplazamiento en el eje Z
};

// Representa la ficha cuántica completa de una frecuencia de espectroscopía IR
struct Frecuencias {
    float                   frecuencia;     // El valor numérico en cm⁻¹ (ej: 1650.1)
    QVector<Vibraciones>    vibraciones;    // Un vector de desplazamiento por cada átomo de la molécula
};

enum ModoEditor {
    ModoSeleccion,
    ModoDibujo,
    ModoCrearEnlace,
    ModoRotacion3D,
};

struct Estado {
    ModoEditor  modoEditor;
    bool        estaVacio;
    bool        estaGuardado;
    bool        estaModificado;
    bool        hayProceso;
};


class View : public QObject
{
    Q_OBJECT

// =========================================================================
// CLASE PRINCIPAL: Constructor y destructor
// =========================================================================
public:
    explicit View(QObject *parent = nullptr);
    ~View();

// =========================================================================
// FUNCIONES PUBLICAS
// =========================================================================
    void initUi();


signals:
// =========================================================================
// LA SEÑAL: View avisa al exterior transmitiendo su estado interno
// =========================================================================
    void actualizarVista(Estado estado);


private:
// =========================================================================
// FUNCIONES PRIVADAS
// =========================================================================
    void resetValores();

// =========================================================================
// ZONA: ALMACÉN FÍSICO DE VARIABLES PRIVADAS (ESTADOS Y MEMORIA EN RAM)
// =========================================================================

    Estado              m_estado;
    QVector<Atomo>      m_listaAtomos;
    QVector<Enlace>     m_listaEnlaces;


};

#endif // VIEW_H
