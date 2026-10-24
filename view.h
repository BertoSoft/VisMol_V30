#ifndef VIEW_H
#define VIEW_H

#include <QObject>
#include <QVector3D>
#include <QStack>

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

enum TipoCalculo {
    MopacOpt,
    GausssianOpt,
    MopacTrayectoria,
    GAussianTrayectoria,
    MopacFreq,
    GaussianFreq
};


struct ValoresCalculo {
    QString energia         = "-- u.a.";
    QString calorFormacion  = "-- kcal/mol";
    QString ionizacion      = "-- eV";
    QString simetria        = "--";
    QString dipolo          = "-- D";
    QString tiempoCpu       = "-- s";
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
    ValoresCalculo  valoresCalculo;
};

// 1. Estructura limpia para almacenar cada modo vibracional
struct ModoVibracional {
    int numero;                         // Índice del modo (1, 2, 3...)
    float frecuencia;                   // En cm^-1 (Eje X)
    float intensidad;                   // En Debye^2/Angstrom^2 (Eje Y)
    QVector<QVector3D> desplazamientos; // Un vector (dx, dy, dz) por cada átomo
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
    bool        estaEmpezado;
    bool        estaGuardado;
    bool        estaModificado;
    bool        hayProceso;
    bool        estaOptimizado;
    bool        hayDeshacer;
    bool        hayRehacer;
    bool        hayTrayectoria;
    bool        hayFrecuencias;
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
    void            initUi();

    void            nuevoProyecto();
    void            abrirProyecto(const QString& ruta);
    void            guardarProyecto();
    void            cerrarProyecto();
    void            lanzarCalculoQm();


    Estado          getEstado();
    ValoresCalculo  getValoresCalculo();
    QString         getAtomoActivo();
    QVector<Atomo>  getListaAtomos();
    QVector<Enlace> getListaEnlaces();
    int             getIdAtomoSeleccionado();
    QString         getComentario();
    QString         getRutaActual();
    QString         getArgumentos();
    QVector<Trayectoria>    getPelicula() const;
    QVector<float>  getCalores();

    int             getNumeroFotogramas();
    int             getFotogramaActual();
    void            setFotogramaActual(int fotograma);
    void            fotogramaAdelante();
    void            fotogramaAtras();

    QVector<ModoVibracional> getModosVibracionales() const;
    void            actualizarFaseVibracion(int indiceModo, float faseSenoidal, float escalaVisual);


    void            limpiarLienzo();
    void            setElementoActivo(const QString& simbolo);
    void            setModoEditor(ModoEditor modo);
    void            setComentario(const QString& comentario);
    void            setArgumentos(const QString& argumentos);
    void            setRutaActual(const QString& ruta);
    void            setClick(const QVector3D& posClick);
    void            setClickDerecho(const QVector3D& posClick);
    void            eliminarAtomo(int idAtomo);
    void            eliminarEnlace(int idEnlace);

    void            deshacer();
    void            rehacer();


signals:
// =========================================================================
// LA SEÑAL: View avisa al exterior transmitiendo su estado interno
// =========================================================================
    void actualizarVista(const Estado& estado);
    void actualizarLienzo();
    void setMenuEliminar(int idAtomo, int idEnlace);


private:
// =========================================================================
// FUNCIONES PRIVADAS
// =========================================================================
    void            resetValores();
    void            notificaEstado();
    void            procesarClickSeleccion(const QVector3D& posClick);
    void            procesarClickDibujo(const QVector3D& posClick);
    void            procesarClickCrearEnlace(const QVector3D& posClick);
    void            procesarClickRotacion(const QVector3D& posClick);
    int             getIdAtomoFromPosicion(const QVector3D& posClick);
    float           getDistanciaPuntoSegmento(const QVector3D& posClick, const QVector3D& pos1, const QVector3D& pos2);
    int             getEnlaces(const Atomo& atomo);
    bool            isEnlacePosible(const Atomo& atomo1, const Atomo& atomo2);
    void            setEstadoHistorial();
    void            atrasHistorial();
    void            adelanteHistorial();
    void            cargarMopacResultados(const QString& rutaOut, TipoCalculo tipoCalculo);
    TipoCalculo     getTipoCalculo();
    void            lanzarCalculoMopac(const QString& ruta, TipoCalculo tipoCalculo);
    void            lanzarCalculoGaussian(const QString& ruta, TipoCalculo tipoCalculo);
    void            centrarListaAtomos(QVector<Atomo>& lista);



// =========================================================================
// ZONA: ALMACÉN FÍSICO DE VARIABLES PRIVADAS (ESTADOS Y MEMORIA EN RAM)
// =========================================================================

    struct FotoActual{
        QVector<Atomo>      listaAtomos;
        QVector<Enlace>     listaEnlaces;
    };

    Estado                  m_estado;
    QVector<Atomo>          m_listaAtomos;
    QVector<Enlace>         m_listaEnlaces;

    QVector<Atomo>          m_listaAtomosTrabajo; // 🎯 ¡La que lee el lienzo! Variable y dinámica
    QVector<ModoVibracional> m_modosVibracionales; // Historial de modos cargados

    QVector<float>          m_calores;

    QVector<Trayectoria>    m_pelicula;
    int                     m_fotogramaActual;

    int                     m_idAtomoSeleccionado;
    QVector3D               m_ultimaPosRat;
    int                     m_contadorId;
    QString                 m_atomoActivo;
    QString                 m_comentario;
    QString                 m_argumentos;
    QString                 m_rutaArchivoActual;
    ValoresCalculo          m_valoresCalculo;

    int                     m_numeroAtomos;
    int                     m_numeroEnlaces;

    QStack<FotoActual>      m_historialDeshacer;
    QStack<FotoActual>      m_historialRehacer;


};

#endif // VIEW_H
