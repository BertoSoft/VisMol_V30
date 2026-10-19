#include "vista.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Vista w;
    w.show();
    return QApplication::exec();
}
