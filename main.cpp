#include "vista.h"
#include "lienzo.h"

#include <QApplication>
#include <QObject>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Vista*  vista   = new Vista();
    View*   view    = new View();

    // 🎯 SOLUCIÓN: Buscamos el lienzo real que se creó dentro del visor molecular de tu UI
    Lienzo* lienzo = vista->findChild<Lienzo*>("visorMolecular");

    vista->initView(view);
    vista->initLienzo(lienzo);


    lienzo->initView(view);


    //======================================================================================
    // Conexiones de señales
    //=======================================================================================
    QObject::connect(view, &View::actualizarVista, vista, &Vista::actualizarUi);
    QObject::connect(view, &View::actualizarLienzo, lienzo, &Lienzo::renderLienzo);


    view->initUi(); //
    vista->show();  // Forzamos el renderizado inicial básico
    QTimer::singleShot(10, vista, &QWidget::showMaximized);

    return QApplication::exec(); //
}
