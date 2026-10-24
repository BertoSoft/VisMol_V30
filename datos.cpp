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

    listaAtomos.clear();
    listaEnlaces.clear();

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
            if (linea.isEmpty() && !posicionesFinales.isEmpty()) {
                encontradoBloqueGeometria = false;
                continue;
            }

            QStringList columnas = linea.split(' ', Qt::SkipEmptyParts);

            // El formato real de MOPAC con flags es:
            // [0]=Símbolo, [1]=X, [2]=FlagX, [3]=Y, [4]=FlagY, [5]=Z, [6]=FlagZ
            if (columnas.size() >= 6) {
                bool okX, okY, okZ;

                float xAngstrom = columnas[1].toFloat(&okX);
                float yAngstrom = columnas[3].toFloat(&okY);
                float zAngstrom = columnas[5].toFloat(&okZ);

                if (okX && okY && okZ) {
                    // 📐 FACTOR_ANGSTROM ÚNICO: 1 Ángstrom = 100 píxeles de escena
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

bool Datos::leerMopacOutSoloCalores(
    QTextStream& streamOut,
    QVector<float>& m_calores
){
    m_calores.clear();

    // Nos aseguramos de posicionar el puntero al inicio del archivo por protocolo
    streamOut.device()->seek(0);

    while (!streamOut.atEnd()) {
        QString linea = streamOut.readLine().trimmed();

        // Buscamos las líneas de iteración del optimizador geométrico de MOPAC
        if (linea.startsWith("CYCLE:") && linea.contains("HEAT:")) {
            // Ejemplo: "CYCLE:    20 TIME:   0.004 TIME LEFT:  2.00D  GRAD.:    67.387 HEAT: -1.092362"
            // Cortamos el string para quedarnos con el fragmento que va después de "HEAT:"
            QString valorStr = linea.section("HEAT:", 1).simplified();

            if (!valorStr.isEmpty()) {
                bool ok;
                float valorCalor = valorStr.toFloat(&ok);
                if (ok) {
                    m_calores.append(valorCalor);
                }
            }
        }
    }

    return !m_calores.isEmpty();
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

    QVector<QString> listaCaloresIntermedios;

    // Aseguramos que el stream comience desde el principio del archivo físico .aux
    stream.device()->seek(0);

    while (!stream.atEnd()) {
        QString linea = stream.readLine().trimmed();

        // 🎯 CAPTURA DIRECTA Y EXACTA: Usamos el tag real detectado en tu .aux
        if (linea.startsWith("HEAT_OF_FORM_UPDATED:KCAL/MOL=")) {
            // Aislamos la parte numérica que va justo después del signo '='
            QString valorStr = linea.section('=', 1).trimmed();

            if (!valorStr.isEmpty()) {
                // 🛠️ PARSER DE CÓMPUTO QUÍMICO: MOPAC escribe 'D' para exponentes (ej: +0.342128D+04)
                // C++ no entiende la 'D', por lo que la mutamos a 'E' para permitir el parseo flotante.
                QString stringSincronizado = valorStr;
                stringSincronizado.replace('D', 'E', Qt::CaseInsensitive);

                bool ok;
                float valorEnergia = stringSincronizado.toFloat(&ok);

                if (ok) {
                    // Volvemos a formatear a un string amigable para presentarlo en el QLabel de la UI
                    listaCaloresIntermedios.append(QString("%1 kcal/mol").arg(valorEnergia, 0, 'f', 4));
                } else {
                    // Salvaguarda: si por alguna razón falla el float, guardamos el texto original limpio
                    listaCaloresIntermedios.append(QString("%1 kcal/mol").arg(valorStr));
                }
            }
        }

        // 🎯 FASE 2: Captura y reconstrucción geométrica de las coordenadas de los fotogramas
        if (linea.startsWith("ATOM_X_UPDATED:ANGSTROMS") || linea.startsWith("ATOM_X_OPT:ANGSTROMS")) {
            Trayectoria fotograma;
            QVector<float> coordenadasBloque;
            int valoresLeidos = 0;

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

            if (valoresLeidos == valoresEsperados) {
                const float PIXELES_POR_ANGSTROM = 100.0f;
                int indiceCoordenada = 0;

                for (int i = 0; i < numAtomos; ++i) {
                    Atomo at;
                    at.id      = listaAtomosBase[i].id;
                    at.simbolo = listaAtomosBase[i].simbolo;

                    float x = coordenadasBloque[indiceCoordenada++] * PIXELES_POR_ANGSTROM;
                    float y = coordenadasBloque[indiceCoordenada++] * PIXELES_POR_ANGSTROM;
                    float z = coordenadasBloque[indiceCoordenada++] * PIXELES_POR_ANGSTROM;
                    at.posicion = QVector3D(x, y, z);

                    fotograma.listaAtomos.append(at);
                }

                // Inicializamos los valores por defecto que se rellenarán en la pasada final
                fotograma.valoresCalculo.energia        = "-- u.a.";
                fotograma.valoresCalculo.calorFormacion = "-- kcal/mol";
                fotograma.valoresCalculo.ionizacion     = "-- eV";
                fotograma.valoresCalculo.simetria       = "--";
                fotograma.valoresCalculo.dipolo         = "-- D";
                fotograma.valoresCalculo.tiempoCpu      = "-- s";

                pelicula.append(fotograma);
            }
        }
    }

    // 🎯 FASE 3: Mapeado síncrono del histórico de energías en los fotogramas en memoria
    for (int i = 0; i < pelicula.size(); ++i) {
        if (i < listaCaloresIntermedios.size()) {
            pelicula[i].valoresCalculo.calorFormacion = listaCaloresIntermedios[i];
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

            // Aseguramos que la línea tiene las columnas requeridas: Índice, Símbolo, X, Y, Z
            if (columnas.size() >= 5) {
                bool okX, okY, okZ;

                // El formato del .out es limpio: columnas[2]=X, columnas[3]=Y, columnas[4]=Z
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

