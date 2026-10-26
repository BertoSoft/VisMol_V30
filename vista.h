#ifndef VISTA_H
#define VISTA_H

#include "view.h"
#include "lienzo.h"


#include <QMainWindow>
#include <QButtonGroup> // Necesario para la exclusividad de los botones
#include <QLabel>
#include <QListWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui {
class Vista;
}
QT_END_NAMESPACE

class Vista : public QMainWindow
{
    Q_OBJECT

public:
    explicit Vista(QWidget *parent = nullptr);
    ~Vista() override;

    const QString NOMBRE_APP = "VisMol V 3.0";

// =========================================================================
// BLOQUE: FUNCIONES PUBLICAS PARA SER LLAMADAS POR VIEW
// =========================================================================
    void initView(View* view);
    void initLienzo(Lienzo* lienzo);

// =========================================================================
// BLOQUE: SLOTS DE SEÑALES PUBLICOS
// =========================================================================
public slots:
    // Slots de sincronización descendente que llamará MoleculaView mediante señales
    void actualizarUi(Estado estado);
    void setMenuBotonDerecho(int idAtomo, int idEnlace);


private slots:
    // =========================================================================
    // BLOQUE: GESTIÓN DE BOTONERÍA Y CONTROLES INTERNOS DE INTERFAZ
    // =========================================================================
    void on_actionSalir_triggered();

    void on_actionNuevo_triggered();

    void on_actionCerrar_triggered();

    void on_actionAbrir_triggered();

    void on_actionGuardar_triggered();

    void on_btnCarbono_clicked();

    void on_btnHidrogeno_clicked();

    void on_btnOxigeno_clicked();

    void on_btnNitrogeno_clicked();

    void on_btnAzufre_clicked();

    void on_btnSeleccion_clicked();

    void on_btnModoEnlace_clicked();

    void on_btnModoRotar_clicked();

    void on_actionDeshacer_triggered();

    void on_actionRehacer_triggered();

    void on_actionLimpiarLienzo_triggered();

    void on_actionComentarioProyecto_triggered();

    void on_actionGuardarComo_triggered();

    void on_btnConfigurarParametros_clicked();

    void on_btnLanzar_clicked();

    void on_btnPasoAtras_clicked();

    void on_btnPasoAdelante_clicked();

    void on_sliderPasos_valueChanged(int value);

    void on_btnPlayAnimacion_clicked();

    void on_listaFrecuencias_itemClicked(QListWidgetItem *item);

    void on_comboSoftware_activated(int index);

private:
    Ui::Vista *ui;

    // =========================================================================
    // BLOQUE: MÉTODOS DE INICIALIZACIÓN POR CAPAS MODULARES
    // =========================================================================
    void initUi();
    void initGrupoElementos();
    void initBarraEstado();
    void initAnimacion();

    //=========================================================================
    // BLOQUE: FUNCIONES Y VARIABLES PRIVADAS
    //=========================================================================
    void setEstilos();
    bool sePuedeCerrar();
    void dialogoComentario();
    void configurarCalculoMopac();
    void configurarCalculoOrca();
    void rellenarListaFrecuencias();
    void actualizarFrameVibracion();
    void stopAnimaciones(const Estado& estado);

    // 🎯 NUEVOS SUB-MÓDULOS DE SINCRONIZACIÓN DE INTERFAZ (REFALM)
    void actualizarTituloYMetadatos(const Estado& estado);
    void actualizarAccionesYMenus(const Estado& estado);
    void actualizarControlesEdicion(const Estado& estado);
    void actualizarPanelResultados(const Estado& estado);
    void actualizarGraficaEInferior(const Estado& estado);

    int             m_tipoCalculo       = -1;
    int             m_programa          = -1;

    View*           m_view              = nullptr;
    Lienzo*         m_lienzo            = nullptr;
    QButtonGroup*   m_grupoIzdo         = nullptr;

    QLabel*         m_lblMesaje         = nullptr;
    QLabel*         m_fecha             = nullptr;
    QLabel*         m_hora              = nullptr;

    QTimer*         m_timerAnimacion    = nullptr;

    QTimer*         m_timerVibracion    = nullptr; // Temporizador dedicado al motor cuántico
    float           m_faseVibracion     = 0.0f;    // Ángulo oscilador continuo
    int             m_idxModoActivo     = -1;      // Índice de la RAM que se está moviendo
    float           m_escalaVisual      = 1.0f;    // Amplitud para exagerar el movimiento del enlace


};
#endif // VISTA_H