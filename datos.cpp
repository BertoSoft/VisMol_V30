#include "datos.h"

#include <QFile>
#include <QIODevice>
#include <QDateTime>
#include <QTextStream>


bool Datos::guardarVisMol(
    const QString& ruta,
    const QVector<Atomo>& listaAtomos,
    const QVector<Enlace>& listaEnlaces,
    const QString& comentario
    ){

    QFile archivo(ruta);
    if (!archivo.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&archivo);

    // 1. Primera línea: Estampa de tiempo (Fecha y hora actual)
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";

    // 2. Segunda línea: Nota o comentario asociado a la molécula
    out << comentario << "\n";

    // 3. Bloque [Atomos]: ID Símbolo X Y Z (con 6 decimales fijos)
    out << "[Atomos]\n";
    for (const Atomo& a : listaAtomos) {
        out << a.id << " " << a.simbolo << " "
            << qSetRealNumberPrecision(6) << qSetFieldWidth(0) << Qt::fixed
            << a.posicion.x() << " "
            << a.posicion.y() << " "
            << a.posicion.z() << "\n";
    }

    // 4. Bloque [Enlaces]: ID_Origen ID_Destino Orden
    out << "[Enlaces]\n";
    for (const Enlace& e : listaEnlaces) {
        out << e.id_atomo1 << " " << e.id_atomo2 << " " << e.orden << "\n";
    }

    archivo.close();

    return true;
}

bool Datos::abrirVisMol(
    const QString& ruta,
    QVector<Atomo>& listaAtomos,
    QVector<Enlace>& listaEnlaces,
    QString& comentario
    ){

    QFile archivo(ruta);
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&archivo);

    // 1. Leer metadatos iniciales de las dos primeras líneas
    QString estampaTiempo = in.readLine(); // Línea 1: Estampa temporal de guardado
    comentario = in.readLine();          // Línea 2: Notas de la molécula

    QString seccionActual = "";

    // 2. Parsear los bloques de datos mediante bucle de lectura
    while (!in.atEnd()) {
        QString linea = in.readLine().trimmed();
        if (linea.isEmpty()) continue;

        // Detectar cabeceras de sección
        if (linea == "[Atomos]") {
            seccionActual = "[Atomos]";
            continue;
        } else if (linea == "[Enlaces]") {
            seccionActual = "[Enlaces]";
            continue;
        }

        QTextStream parser(&linea);

        if (seccionActual == "[Atomos]") {
            Atomo a;
            float x, y, z;
            parser >> a.id >> a.simbolo >> x >> y >> z;
            a.posicion = QVector3D(x, y, z);

            listaAtomos.append(a);
        }
        else if (seccionActual == "[Enlaces]") {
            Enlace e;
            parser >> e.id_atomo1 >> e.id_atomo2 >> e.orden;

            listaEnlaces.append(e);
        }
    }

    archivo.close();
    return true;
}

bool Datos::leerOutOptNoInter(QTextStream& stream, QVector<Atomo>& listaAtomos){
    QStringList lineas;
    int indiceJobEnded = -1;

    // 1. Volcar todo el flujo de texto en una lista de líneas para buscar hacia atrás
    while (!stream.atEnd()) {
        lineas.append(stream.readLine());
    }

    // 2. Buscar desde el final la confirmación de éxito de MOPAC
    for (int i = lineas.size() - 1; i >= 0; --i) {
        if (lineas[i].contains("JOB ENDED NORMALLY")) {
            indiceJobEnded = i;
            break;
        }
    }

    if (indiceJobEnded == -1) return false;

    int indiceCartesianCoordinates = -1;
    // 3. Volver hacia atrás desde el éxito hasta la sección de coordenadas finales
    for (int i = indiceJobEnded; i >= 0; --i) {
        // Buscamos la etiqueta limpia sin la palabra ANGSTROMS
        if (lineas[i].contains("CARTESIAN COORDINATES") && !lineas[i].contains("ANGSTROMS")) {
            indiceCartesianCoordinates = i;
            break;
        }
    }

    if (indiceCartesianCoordinates == -1) return false;

    QVector<QVector3D> coordenadasOptimadas;

    // 4. Parsear la tabla saltándonos las líneas en blanco iniciales de separación de MOPAC
    for (int i = indiceCartesianCoordinates + 1; i < lineas.size(); ++i) {
        QString linea = lineas[i].trimmed();

        // 🎯 ¡LA CORRECCIÓN!: Si la línea está vacía pero aún no hemos leído ningún átomo,
        // simplemente nos la saltamos en lugar de romper el bucle.
        if (linea.isEmpty()) {
            if (coordenadasOptimadas.isEmpty()) {
                continue; // Salta la línea en blanco entre la cabecera y el átomo 1
            } else {
                break; // Si ya habíamos leído átomos, una línea en blanco sí marca el fin de la tabla
            }
        }

        QTextStream parser(&linea);
        int numeroAtomo;
        QString simbolo;
        float x, y, z;

        // Formato final de alta precisión de MOPAC: ID  Símbolo  X  Y  Z
        parser >> numeroAtomo >> simbolo >> x >> y >> z;

        // Si la línea contiene texto basura que no es un átomo, abortamos de forma segura
        if (simbolo.isEmpty() || numeroAtomo <= 0) continue;

        coordenadasOptimadas.append(QVector3D(x, y, z));
    }

    // 5. Aplicar la constante inversa del escalado (1 Angstrom = 100 píxeles de escena)
    const float PIXELES_POR_ANGSTROM = 100.0f;

    if (!coordenadasOptimadas.isEmpty() && coordenadasOptimadas.size() == listaAtomos.size()) {
        for (int i = 0; i < listaAtomos.size(); ++i) {
            // Multiplicamos por 100 para transformar los Angstroms cuánticos a píxeles nítidos
            listaAtomos[i].posicion = coordenadasOptimadas[i] * PIXELES_POR_ANGSTROM;
        }
        return true; // Éxito, las posiciones definitivas mutan en la RAM
    }

    return false;
}