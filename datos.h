#ifndef DATOS_H
#define DATOS_H

#include "view.h"


#include <QString>


class Datos{
public:
    Datos()     = delete;
    ~Datos()    = delete;

    static bool guardarVisMol(
        const QString& ruta,
        const QVector<Atomo>& listaAtomos,
        const QVector<Enlace>& listaEnlaces,
        const QString& comentario
    );

    static bool abrirVisMol(
        const QString& ruta,
        QVector<Atomo>& listaAtomos,
        QVector<Enlace>& listaEnlaces,
        QString& comentario
    );

    static bool leerMopacArcSummary(
        QTextStream& stream,
        QVector<Atomo>& listaAtomos,
        ValoresCalculo& valores
    );

    static bool leerMopacOutSoloCalores(
        QTextStream& streamOut,
        QVector<float>& m_calores
    );

    static bool leerMopacAuxTrayectoria(
        QTextStream& stream,
        QVector<Trayectoria>& pelicula,
        const QVector<Atomo>& listaAtomosBase
        );

    static bool leerMopacOutOptNoInter(
        QTextStream& stream,
        QVector<Atomo>& listaAtomos,
        ValoresCalculo& valores
    );



};

#endif // DATOS_H
