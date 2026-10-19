#ifndef VISTA_H
#define VISTA_H

#include "lienzo.h"
#include "view.h"

#include <QMainWindow>
#include <QButtonGroup> // Necesario para la exclusividad de los botones

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

private:
    Ui::Vista *ui;

    // =========================================================================
    // BLOQUE: MÉTODOS DE INICIALIZACIÓN POR CAPAS MODULARES
    // =========================================================================
    void initUi();
    void initGrupoElementos();
    void initEscena();

    //=========================================================================
    // BLOQUE: FUNCIONES Y VARIABLES PRIVADAS
    //=========================================================================
    void setEstilos();

    Lienzo*         m_lienzo            = nullptr;
    View*           m_view              = nullptr;
    QButtonGroup*   m_grupoIzdo         = nullptr;

};
#endif // VISTA_H