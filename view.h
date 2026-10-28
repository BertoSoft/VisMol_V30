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
    MopacTrayectoria,
    MopacFreq,
    OrcaOpt,
    OrcaTrayectoria,
    OrcaFreq
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
    ModoMedida
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
    QString     medida;
    QString     angulo;
    QString     anguloDihedro;
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
    void            lanzarCalculoQm(int programa, int calculo);
    void            exportarOrca(const QString& ruta);
    void            exportarMopac(const QString& ruta);
    void            importarORCA(const QString& rutaXyz, int calculo);
    void            importarMopac(const QString& rutaFichero, int calculo); // ◄ TU NUEVO CONMUTADOR DE TRES VÍAS
    void            cargarXyzORCA(const QString& rutaXyz, int calculo);

    Estado          getEstado();
    ValoresCalculo  getValoresCalculo();
    QString         getAtomoActivo();
    QVector<Atomo>  getListaAtomos();
    QVector<Enlace> getListaEnlaces();
    int             getIdAtomoSeleccionado();
    QString         getComentario();
    QString         getRutaActual();
    QString         getArgumentosMopac();
    QString         getArgumentosOrca();

    int             getIdAtomoMedida1() const;
    int             getIdAtomoMedida2() const;
    int             getIdAtomoMedida3() const;
    int             getIdAtomoMedida4() const;


    QVector<Trayectoria>    getPelicula() const;
    QVector<float>  getCalores();

    int             getNumeroFotogramas();
    int             getFotogramaActual();
    void            setFotogramaActual(int fotograma);
    void            fotogramaAdelante();
    void            fotogramaAtras();

    QVector<ModoVibracional> getModosVibracionales() const;
    void            actualizarFaseVibracion(int indiceModo, float faseSenoidal, float escalaVisual);
    void            stopProceso();

    void            limpiarLienzo();
    void            setElementoActivo(const QString& simbolo);
    void            setModoEditor(ModoEditor modo);
    void            setComentario(const QString& comentario);
    void            setArgumentosMopac(const QString& argumentos);
    void            setArgumentosOrca(const QString& argumentos);

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
    void            procesarClickMedida(const QVector3D& posClick);
    void            hacerMedida(int idAtomo4);
    int             getIdAtomoFromPosicion(const QVector3D& posClick);
    Atomo           getAtomo(int id) const;
    float           getDistanciaPuntoSegmento(const QVector3D& posClick, const QVector3D& pos1, const QVector3D& pos2);
    int             getEnlaces(const Atomo& atomo);
    bool            isEnlacePosible(const Atomo& atomo1, const Atomo& atomo2);
    void            setEstadoHistorial();
    void            atrasHistorial();
    void            adelanteHistorial();
    void            cargarMopacResultados(const QString& rutaOut, TipoCalculo tipoCalculo);
    void            cargarOrcaResultados(const QString& rutaOut, TipoCalculo tipoCalculo);
    TipoCalculo     getTipoCalculo(int programa, int calculo);
    void            lanzarCalculoMopac(const QString& ruta, TipoCalculo tipoCalculo);
    void            lanzarCalculoOrca(const QString& ruta, TipoCalculo tipoCalculo);
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

    QString                 m_argumentosMopac;
    QString                 m_argumentosOrca;
    int                     m_pidProceso;

    QString                 m_rutaArchivoActual;
    ValoresCalculo          m_valoresCalculo;

    int                     m_numeroAtomos;
    int                     m_numeroEnlaces;

    QStack<FotoActual>      m_historialDeshacer;
    QStack<FotoActual>      m_historialRehacer;

    int                     m_idAtomoMedida1;
    int                     m_idAtomoMedida2;
    int                     m_idAtomoMedida3;
    int                     m_idAtomoMedida4;



};

#endif // VIEW_H
