#include "datos.h"

#include <QFile>
#include <QIODevice>
#include <QDateTime>
#include <QTextStream>
#include <QStringList>


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

bool Datos::leerMopacArcSummary(
    QTextStream& stream,
    QVector<Atomo>& listaAtomos,
    ValoresCalculo& valores
    ){
    bool exitoCalculo = false;
    bool encontradoBloqueGeometria = false;

    QVector<QVector3D> posicionesFinales;

    while (!stream.atEnd()) {
        QString linea = stream.readLine().trimmed();

        // 1. Extracción de Magnitudes Cuánticas (Bloque Fijo del Summary)
        if (linea.contains("HEAT OF FORMATION")) {
            QStringList partes = linea.split('=', Qt::SkipEmptyParts);
            if (partes.size() > 1) {
                QStringList subPartes = partes[1].trimmed().split(' ', Qt::SkipEmptyParts);
                if (!subPartes.isEmpty()) {
                    valores.calorFormacion = QString("%1 kcal/mol").arg(subPartes.first());
                    exitoCalculo = true;
                }
            }
        }
        else if (linea.contains("HOMO LUMO ENERGIES (EV)")) {
            QStringList partes = linea.split('=', Qt::SkipEmptyParts);
            if (partes.size() > 1) {
                QStringList subPartes = partes[1].trimmed().split(' ', Qt::SkipEmptyParts);
                if (!subPartes.isEmpty()) {
                    valores.energia = QString("%1 eV (HOMO)").arg(subPartes.first());
                }
            }
        }
        else if (linea.contains("IONIZATION POTENTIAL")) {
            QStringList partes = linea.split('=', Qt::SkipEmptyParts);
            if (partes.size() > 1) {
                QStringList subPartes = partes[1].trimmed().split(' ', Qt::SkipEmptyParts);
                if (!subPartes.isEmpty()) {
                    valores.ionizacion = QString("%1 eV").arg(subPartes.first());
                }
            }
        }
        // 🎯 CORREGIDO: Usamos contains() en lugar de startsWith() para evitar fallos con los espacios
        else if (linea.contains("DIPOLE") && linea.contains("POINT GROUP:")) {
            // Extraemos el dipolo separando por el '='
            QStringList partesDipolo = linea.split('=', Qt::SkipEmptyParts);
            if (partesDipolo.size() > 1) {
                QStringList subPartesDipolo = partesDipolo[1].trimmed().split(' ', Qt::SkipEmptyParts);
                if (!subPartesDipolo.isEmpty()) {
                    valores.dipolo = QString("%1 D").arg(subPartesDipolo.first());
                }
            }

            // Extraemos el grupo puntual del final aislándolo de forma segura
            QStringList partesSimetria = linea.split("POINT GROUP:", Qt::SkipEmptyParts);
            if (partesSimetria.size() > 1) {
                valores.simetria = partesSimetria[1].trimmed();
            }
        }
        else if (linea.contains("COMPUTATION TIME")) {
            QStringList partes = linea.split('=', Qt::SkipEmptyParts);
            if (partes.size() > 1) {
                QStringList subPartes = partes[1].trimmed().split(' ', Qt::SkipEmptyParts);
                if (!subPartes.isEmpty()) {
                    valores.tiempoCpu = QString("%1 s").arg(subPartes.first());
                }
            }
        }

        // 2. Activación del bloque de Geometría Definitiva
        if (linea == "FINAL GEOMETRY OBTAINED") {
            encontradoBloqueGeometria = true;
            stream.readLine(); // Saltamos la línea de keywords redundantes
            stream.readLine(); // Saltamos la línea de título
            continue;
        }

        // 3. Procesamiento de la matriz atómica final
        if (encontradoBloqueGeometria) {
            // Si la línea está vacía y ya tenemos átomos guardados, salimos del bloque
            if (linea.isEmpty() && !posicionesFinales.isEmpty()) {
                encontradoBloqueGeometria = false;
                continue;
            }

            QStringList columnas = linea.split(' ', Qt::SkipEmptyParts);
            // El formato real en el .arc es: [0]=Símbolo, [1]=X, [2]=Flag, [3]=Y, [4]=Flag, [5]=Z, [6]=Flag
            if (columnas.size() >= 6) {
                bool okX, okY, okZ;
                float xAngstrom = columnas[1].toFloat(&okX);
                float yAngstrom = columnas[3].toFloat(&okY);
                float zAngstrom = columnas[5].toFloat(&okZ);

                if (okX && okY && okZ) {
                    const float PIXELES_POR_ANGSTROM = 100.0f;
                    posicionesFinales.append(QVector3D(xAngstrom * PIXELES_POR_ANGSTROM,
                                                       yAngstrom * PIXELES_POR_ANGSTROM,
                                                       zAngstrom * PIXELES_POR_ANGSTROM));
                }
            }
        }
    }

    // 4. Volcado síncrono en la RAM si las dimensiones coinciden
    if (exitoCalculo && posicionesFinales.size() == listaAtomos.size()) {
        for (int i = 0; i < listaAtomos.size(); ++i) {
            listaAtomos[i].posicion = posicionesFinales[i];
        }
        return true;
    }

    return false;
}

bool Datos::leerMopacAuxTrayectoria(
    QTextStream& stream,
    QVector<Trayectoria>& pelicula,
    const QVector<Atomo>& listaAtomosBase
    ){
    pelicula.clear();
    if (listaAtomosBase.isEmpty()) return false;

    int numAtomos = listaAtomosBase.size();
    int valoresEsperados = numAtomos * 3; // X, Y, Z por cada átomo del sistema

    while (!stream.atEnd()) {
        QString linea = stream.readLine().trimmed();

        // 🎯 CAPTURA DE FOTOGRAMAS: Detectamos tanto los pasos intermedios como el óptimo final
        if (linea.startsWith("ATOM_X_UPDATED:ANGSTROMS") || linea.startsWith("ATOM_X_OPT:ANGSTROMS")) {

            Trayectoria fotograma;
            QVector<float> coordenadasBloque;
            int valoresLeidos = 0;

            // Recolectamos las líneas de números flotantes que componen este fotograma
            while (!stream.atEnd() && valoresLeidos < valoresEsperados) {
                QString subLinea = stream.readLine().trimmed();
                if (subLinea.isEmpty() || subLinea.startsWith("#")) break;

                QStringList tokens = subLinea.split(' ', Qt::SkipEmptyParts);
                for (const QString& token : tokens) {
                    bool ok;
                    float val = token.toFloat(&ok);
                    if (ok) {
                        coordenadasBloque.append(val);
                        valoresLeidos++;
                    }
                }
            }

            // Si el bloque se ha leído por completo, reconstruimos la estructura 3D del frame
            if (valoresLeidos == valoresEsperados) {
                const float PIXELES_POR_ANGSTROM = 100.0f;
                int indiceCoordenada = 0;

                for (int i = 0; i < numAtomos; ++i) {
                    Atomo at;
                    // Heredamos de forma síncrona el ID y Símbolo del átomo original de la RAM
                    at.id      = listaAtomosBase[i].id;
                    at.simbolo = listaAtomosBase[i].simbolo;

                    // Extraemos y escalamos el vector de posición (de Angstroms a Píxeles)
                    float x = coordenadasBloque[indiceCoordenada++] * PIXELES_POR_ANGSTROM;
                    float y = coordenadasBloque[indiceCoordenada++] * PIXELES_POR_ANGSTROM;
                    float z = coordenadasBloque[indiceCoordenada++] * PIXELES_POR_ANGSTROM;
                    at.posicion = QVector3D(x, y, z);

                    fotograma.listaAtomos.append(at);
                }

                // Añadimos el fotograma completado a la película
                pelicula.append(fotograma);
            }
        }
    }

    return !pelicula.isEmpty();
}

bool Datos::leerMopacOutOptNoInter(
    QTextStream& stream,
    QVector<Atomo>& listaAtomos,
    ValoresCalculo& valores
    ){
    bool encontradoBloqueCoordenadas = false;
    bool exitoCalculo = false;
    bool trabajoFinalizadoOk = false;

    QVector<QVector3D> nuevasPosiciones;

    while (!stream.atEnd()) {
        QString linea = stream.readLine().trimmed();

        // 1. Detectar el fin de trabajo correcto
        if (linea.contains("JOB ENDED NORMALLY")) {
            trabajoFinalizadoOk = true;
        }

        // 2. Extracción rigurosa de magnitudes basada en tu fichero real
        if (linea.contains("FINAL HEAT OF FORMATION")) {
            QStringList partes = linea.split('=', Qt::SkipEmptyParts);
            if (partes.size() > 1) {
                QStringList subPartes = partes[1].trimmed().split(' ', Qt::SkipEmptyParts);
                if (!subPartes.isEmpty()) {
                    valores.calorFormacion = QString("%1 kcal/mol").arg(subPartes.first());
                    exitoCalculo = true;
                }
            }
        }
        else if (linea.contains("HOMO LUMO ENERGIES (EV)")) {
            QStringList partes = linea.split('=', Qt::SkipEmptyParts);
            if (partes.size() > 1) {
                QStringList subPartes = partes[1].trimmed().split(' ', Qt::SkipEmptyParts);
                if (!subPartes.isEmpty()) {
                    valores.energia = QString("%1 eV (HOMO)").arg(subPartes.first());
                }
            }
        }
        else if (linea.contains("IONIZATION POTENTIAL")) {
            QStringList partes = linea.split('=', Qt::SkipEmptyParts);
            if (partes.size() > 1) {
                QStringList subPartes = partes[1].trimmed().split(' ', Qt::SkipEmptyParts);
                if (!subPartes.isEmpty()) {
                    valores.ionizacion = QString("%1 eV").arg(subPartes.first());
                }
            }
        }
        else if (linea.startsWith("SUM") && linea.contains(".")) {
            QStringList columnasDipole = linea.split(' ', Qt::SkipEmptyParts);
            if (columnasDipole.size() >= 5) {
                valores.dipolo = QString("%1 D").arg(columnasDipole.last());
            }
        }
        // 🎯 CORREGIDO: MOPAC escribe "COMPUTATION TIME  =  0.305 SECONDS" en el bloque final
        else if (linea.contains("COMPUTATION TIME")) {
            QStringList partes = linea.split('=', Qt::SkipEmptyParts);
            if (partes.size() > 1) {
                QStringList subPartes = partes[1].trimmed().split(' ', Qt::SkipEmptyParts);
                if (!subPartes.isEmpty()) {
                    valores.tiempoCpu = QString("%1 s").arg(subPartes.first());
                }
            }
        }
        else if (linea.contains("POINT GROUP")) {
            QStringList partes = linea.split(':', Qt::SkipEmptyParts);
            if (partes.size() > 1) {
                valores.simetria = partes[1].trimmed();
            }
        }

        // 3. 🎯 CORREGIDO: Forzar la localización de la geometría OPTIMIZADA FINAL.
        // Solo empezamos a guardar coordenadas si ya hemos leído la convergencia (exitoCalculo es true)
        if (linea == "CARTESIAN COORDINATES" && exitoCalculo) {
            encontradoBloqueCoordenadas = true;
            nuevasPosiciones.clear(); // Limpiamos cualquier residuo previo por seguridad
            stream.readLine(); // Saltamos la línea en blanco reglamentaria
            continue;
        }

        // 4. Procesamiento de la tabla de coordenadas de átomos
        if (encontradoBloqueCoordenadas) {
            if (linea.contains("Empirical Formula") || (linea.isEmpty() && !nuevasPosiciones.isEmpty())) {
                encontradoBloqueCoordenadas = false;
                continue;
            }

            QStringList columnas = linea.split(' ', Qt::SkipEmptyParts);
            // El formato de la última tabla es: [0]=Índice, [1]=Símbolo, [2]=X, [3]=Y, [4]=Z
            if (columnas.size() >= 5) {
                bool okX, okY, okZ;
                float xAngstrom = columnas[2].toFloat(&okX);
                float yAngstrom = columnas[3].toFloat(&okY);
                float zAngstrom = columnas[4].toFloat(&okZ);

                if (okX && okY && okZ) {
                    const float PIXELES_POR_ANGSTROM = 100.0f;
                    nuevasPosiciones.append(QVector3D(xAngstrom * PIXELES_POR_ANGSTROM,
                                                      yAngstrom * PIXELES_POR_ANGSTROM,
                                                      zAngstrom * PIXELES_POR_ANGSTROM));
                }
            }
        }
    }

    // Validación definitiva antes de aplicar cambios a la RAM corporativa
    if (exitoCalculo && trabajoFinalizadoOk && nuevasPosiciones.size() == listaAtomos.size()) {
        for (int i = 0; i < listaAtomos.size(); ++i) {
            listaAtomos[i].posicion = nuevasPosiciones[i];
        }
        return true;
    }

    return false;
}

