#ifndef VISTA_H
#define VISTA_H

#include "view.h"
#include "lienzo.h"


#include <QMainWindow>
#include <QButtonGroup> // Necesario para la exclusividad de los botones
#include <QLabel>

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

private:
    Ui::Vista *ui;

    // =========================================================================
    // BLOQUE: MÉTODOS DE INICIALIZACIÓN POR CAPAS MODULARES
    // =========================================================================
    void initUi();
    void initGrupoElementos();
    void initBarraEstado();

    //=========================================================================
    // BLOQUE: FUNCIONES Y VARIABLES PRIVADAS
    //=========================================================================
    void setEstilos();
    bool sePuedeCerrar();

    View*           m_view              = nullptr;
    Lienzo*         m_lienzo            = nullptr;
    QButtonGroup*   m_grupoIzdo         = nullptr;

    QLabel*         m_lblMesaje         = nullptr;
    QLabel*         m_fecha             = nullptr;
    QLabel*         m_hora              = nullptr;




};
#endif // VISTA_H