#include "vista.h"

#include <QApplication>
#include <QObject>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Vista*  vista   = new Vista();
    View*   view    = new View();
    Lienzo* lienzo  = new Lienzo();

    vista->initView(view);
    vista->initLienzo(lienzo);



    //======================================================================================
    // Conexiones de señales
    //=======================================================================================
    QObject::connect(view, &View::actualizarVista, vista, &Vista::actualizarUi);



    view->initUi();
    vista->showMaximized();

    return QApplication::exec();
}
