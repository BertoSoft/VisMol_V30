#include "vista.h"

#include <QApplication>
#include <QResource>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Q_INIT_RESOURCE(iconos);

    Vista w;
    w.show();
    return QApplication::exec();
}
