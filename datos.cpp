#include "datos.h"

#include <QFile>
#include <QIODevice>
#include <QDateTime>
#include <QTextStream>
#include <QStringList>
#include <QRegularExpression>


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

bool Datos::leerMopacOutSoloFrecuencias(
    QTextStream& streamOut,
    const QVector<Atomo>& listaAtomos,
    QVector<ModoVibracional>& m_modosVibracionales
    ){
    m_modosVibracionales.clear();
    if (listaAtomos.isEmpty()) return false;

    // Volcar el stream completo a un string para buscar los anclajes estructurales
    streamOut.device()->seek(0);
    QString todoElTexto = streamOut.readAll();

    // 1. ANCLAJE: Localizamos el inicio real de la tabla analítica en el .out
    int indexAnalisis = todoElTexto.indexOf("NORMAL COORDINATE ANALYSIS");
    if (indexAnalisis == -1) return false;

    QString seccionFreq = todoElTexto.mid(indexAnalisis);
    QTextStream lector(&seccionFreq);
    lector.readLine(); // Consumimos la línea del título

    // Expresiones regulares adaptadas
    QRegularExpression regexDigitos(R"((-?\d+\.\d+))");
    QRegularExpression regexFilaDesplazamiento(R"(^\s*(\d+)\s+(-?\d+\.\d+.*))");

    QVector<float> todasLasFrecuenciasDetectadas;
    QStringList lineasEstructura;
    QVector<int> mapeoFilaABloqueVertical;
    int contadorBloquesVerticales = 0;

    // =========================================================================
    // 2. ESCANEO DEL BLOQUE COMPLETO DE MODOS NORMALES (ACUMULACIÓN)
    // =========================================================================
    while (!lector.atEnd()) {
        QString linea = lector.readLine();
        QString lineaTrimmed = linea.trimmed();

        if (linea.contains("MASS-WEIGHTED COORDINATE ANALYSIS") || linea.contains("FORCE CONSTANT IN CARTESIAN")) {
            break;
        }

        if (lineaTrimmed.isEmpty()) {
            if (!lineasEstructura.isEmpty() && mapeoFilaABloqueVertical.size() == lineasEstructura.size()) {
                contadorBloquesVerticales++;
            }
            continue;
        }

        // Capturamos de forma continua todas las filas horizontales de frecuencias cm⁻¹
        if (!lineaTrimmed.startsWith("Root") &&
            !lineaTrimmed.contains("A'") && !lineaTrimmed.contains("A\"") &&
            !lineaTrimmed.contains("Ag") && !lineaTrimmed.contains("B1u") &&
            !lineaTrimmed.contains("B2u") && !lineaTrimmed.contains("E2u") &&
            !lineaTrimmed.contains("B1g") && !lineaTrimmed.contains("E2g") &&
            !lineaTrimmed.contains("A2u") && !lineaTrimmed.contains("E1g") &&
            regexDigitos.match(lineaTrimmed).hasMatch() &&
            !regexFilaDesplazamiento.match(linea).hasMatch()) {

            QRegularExpressionMatchIterator itFila = regexDigitos.globalMatch(lineaTrimmed);
            while (itFila.hasNext()) {
                todasLasFrecuenciasDetectadas.append(itFila.next().captured(1).toFloat());
            }
            continue;
        }

        QRegularExpressionMatch matchAtomo = regexFilaDesplazamiento.match(linea);
        if (matchAtomo.hasMatch()) {
            lineasEstructura.append(linea);
            mapeoFilaABloqueVertical.append(contadorBloquesVerticales);
        }
    }

    if (todasLasFrecuenciasDetectadas.isEmpty() || lineasEstructura.isEmpty()) return false;

    // =========================================================================
    // 3. CÁLCULO DINÁMICO DIMENSIONAL DEL SISTEMA
    // =========================================================================
    int maxIdFilaMatriz = 0;
    for (const QString& linea : lineasEstructura) {
        QRegularExpressionMatch match = regexFilaDesplazamiento.match(linea);
        if (match.hasMatch()) {
            int id = match.captured(1).toInt();
            if (id > maxIdFilaMatriz) maxIdFilaMatriz = id;
        }
    }
    int totalLineasCartesianasPorBloque = maxIdFilaMatriz;

    QMap<int, float> mapaFrecuencias;
    QMap<int, QVector<QVector3D>> mapaDesplazamientos;

    for (int i = 0; i < todasLasFrecuenciasDetectadas.size(); ++i) {
        mapaFrecuencias[i] = todasLasFrecuenciasDetectadas[i];
        mapaDesplazamientos[i].resize(listaAtomos.size());
    }

    QMap<int, int> columnasLeidasPorBloqueVertical;

    // =========================================================================
    // 4. EXTRACCIÓN DINÁMICA TRIDIMENSIONAL
    // =========================================================================
    // 📐 FACTOR_ANGSTROM ÚNICO: 1 Ángstrom = 100 píxeles de escena
    const float PIXELES_POR_ANGSTROM = 100.0f;

    for (int i = 0; i < lineasEstructura.size(); ++i) {
        QRegularExpressionMatch match = regexFilaDesplazamiento.match(lineasEstructura[i]);
        if (match.hasMatch()) {
            int bloqueVerticalActual = mapeoFilaABloqueVertical[i];
            int idFilaMatriz = match.captured(1).toInt();

            int idAtomoMapeado = (idFilaMatriz - 1) / 3;
            int ejeActivo = (idFilaMatriz - 1) % 3;

            if (idAtomoMapeado >= listaAtomos.size()) continue;

            QString restoDeLaLinea = match.captured(2);
            QRegularExpressionMatchIterator itColumnas = regexDigitos.globalMatch(restoDeLaLinea);

            int columnaLocalDelBloque = 0;
            while (itColumnas.hasNext()) {
                float valorDesplazamiento = itColumnas.next().captured(1).toFloat();

                // 🎯 LA CLAVE: Multiplicamos el desplazamiento de MOPAC por 100
                // para transformar Ángstroms en píxeles visibles de lienzo
                float valorEscaladoPixel = valorDesplazamiento * PIXELES_POR_ANGSTROM;

                int columnasPreviasTotales = 0;
                for (int b = 0; b < bloqueVerticalActual; ++b) {
                    columnasPreviasTotales += columnasLeidasPorBloqueVertical.value(b, 0);
                }

                int columnaGlobal = columnasPreviasTotales + columnaLocalDelBloque;

                if (!mapaFrecuencias.contains(columnaGlobal)) {
                    columnaLocalDelBloque++;
                    continue;
                }

                // Inyectamos el valor escalado a píxeles según el eje espacial activo
                if (ejeActivo == 0) mapaDesplazamientos[columnaGlobal][idAtomoMapeado].setX(valorEscaladoPixel);
                if (ejeActivo == 1) mapaDesplazamientos[columnaGlobal][idAtomoMapeado].setY(valorEscaladoPixel);
                if (ejeActivo == 2) mapaDesplazamientos[columnaGlobal][idAtomoMapeado].setZ(valorEscaladoPixel);

                if (idFilaMatriz == totalLineasCartesianasPorBloque) {
                    columnasLeidasPorBloqueVertical[bloqueVerticalActual] = columnaLocalDelBloque + 1;
                }

                columnaLocalDelBloque++;
            }
        }
    }

    // Map auxiliar para vincular la posición original global (1-indexed basado en Root No.) con el modo real filtrado
    QMap<int, int> mapaRootNoAModoFiltrado;

    // =========================================================================
    // 5. ENSAMBLAR Y APLICAR FILTRO ESTRICTO TRASLACIONAL / ROTACIONAL
    // =========================================================================
    for (int i = 0; i < todasLasFrecuenciasDetectadas.size(); ++i) {
        float valorCm1 = mapaFrecuencias[i];

        if (valorCm1 < 20.0f && valorCm1 > -20.0f) {
            continue;
        }

        QVector<QVector3D> desplazamientosModo = mapaDesplazamientos[i];
        bool tieneMovimientoReal = false;

        for (const QVector3D& v : desplazamientosModo) {
            if (qAbs(v.x()) > 0.0001f || qAbs(v.y()) > 0.0001f || qAbs(v.z()) > 0.0001f) {
                tieneMovimientoReal = true;
                break;
            }
        }

        if (tieneMovimientoReal) {
            ModoVibracional modo;
            modo.numero = m_modosVibracionales.size() + 1;
            modo.frecuencia = valorCm1;

            // 🎯 CAMBIO 1: Inicializamos con un valor centinela negativo por defecto
            modo.intensidad = -1.0f;

            modo.desplazamientos = desplazamientosModo;

            mapaRootNoAModoFiltrado[i + 1] = m_modosVibracionales.size();
            m_modosVibracionales.append(modo);
        }
    }

    // =========================================================================
    // 6. PARSEO DE INTENSIDADES PARA EL ESPECTRO IR (TRANSITION DIPOLE)
    // =========================================================================
    int indexDesc = todoElTexto.indexOf("DESCRIPTION OF VIBRATIONS");
    if (indexDesc != -1) {
        QString seccionDesc = todoElTexto.mid(indexDesc);
        QTextStream lectorDesc(&seccionDesc);

        QRegularExpression regexVibrationLine(R"(^VIBRATION\s+(\d+))");
        int rootNoActual = -1;

        while (!lectorDesc.atEnd()) {
            QString linea = lectorDesc.readLine().trimmed();

            if (linea.contains("FORCE CONSTANT IN CARTESIAN COORDINATES")) {
                break;
            }

            QRegularExpressionMatch matchVib = regexVibrationLine.match(linea);
            if (matchVib.hasMatch()) {
                rootNoActual = matchVib.captured(1).toInt();
                continue;
            }

            // Si el modo superó el filtro traslacional/rotacional y está en la RAM
            if (rootNoActual >= 0 && mapaRootNoAModoFiltrado.contains(rootNoActual)) {
                if (linea.startsWith("TRANSITION DIPOLE")) {
                    QRegularExpressionMatch matchVal = regexDigitos.match(linea);
                    if (matchVal.hasMatch()) {
                        float dipolo = matchVal.captured(1).toFloat();

                        int idxModoDestino = mapaRootNoAModoFiltrado[rootNoActual];
                        // 🎯 Al sobreescribir aquí, los modos que sí tienen espectro pasan a ser >= 0
                        m_modosVibracionales[idxModoDestino].intensidad = dipolo * dipolo;
                    }
                }
            }
        }
    }

    return !m_modosVibracionales.isEmpty();
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

bool Datos::leerOrcaFinalGeometria(
    QTextStream& stream,
    QVector<Atomo>& listaAtomos,
    ValoresCalculo& valores
    ){
    if (stream.atEnd() || listaAtomos.isEmpty()) return false;

    // 1. Leer primera línea: Número de átomos (ej: 12)
    QString primeraLinea = stream.readLine().trimmed();
    int numAtomsCheck = primeraLinea.toInt();
    if (numAtomsCheck != listaAtomos.size()) return false; // Control de contingencia elemental

    // 2. Leer segunda línea: Extraer la energía del comentario de ORCA
    // Ejemplo: "Coordinates from ORCA-job /tmp/benceno E -232.130424694013"
    QString segundaLinea = stream.readLine().trimmed();
    bool exitoEnergia = false;

    if (segundaLinea.contains(" E ")) {
        QString energiaStr = segundaLinea.section(" E ", 1).trimmed();
        bool ok;
        double energiaHartree = energiaStr.toDouble(&ok);
        if (ok) {
            valores.energia = QString("%1 u.a.").arg(energiaHartree, 0, 'f', 6);
            // Su conversión equivalente a kcal/mol para rellenar el calor de formación
            valores.calorFormacion = QString("%1 kcal/mol").arg(energiaHartree * 627.5093, 0, 'f', 2);
            exitoEnergia = true;
        }
    }

    QVector<QVector3D> posicionesFinales;
    const float PIXELES_POR_ANGSTROM = 100.0f; // Tu escala canónica de renderizado

    // 3. Procesar el bloque de coordenadas XYZ
    while (!stream.atEnd()) {
        QString linea = stream.readLine().trimmed();
        if (linea.isEmpty()) continue;

        QStringList columnas = linea.split(' ', Qt::SkipEmptyParts);
        if (columnas.size() >= 4) { // Símbolo, X, Y, Z
            bool okX, okY, okZ;
            float xAngstrom = columnas[1].toFloat(&okX);
            float yAngstrom = columnas[2].toFloat(&okY);
            float zAngstrom = columnas[3].toFloat(&okZ);

            if (okX && okY && okZ) {
                posicionesFinales.append(QVector3D(xAngstrom * PIXELES_POR_ANGSTROM,
                                                   yAngstrom * PIXELES_POR_ANGSTROM,
                                                   zAngstrom * PIXELES_POR_ANGSTROM));
            }
        }
    }

    // 4. Volcado síncrono en las referencias de la RAM si las dimensiones coinciden
    if (posicionesFinales.size() == listaAtomos.size()) {
        for (int i = 0; i < listaAtomos.size(); ++i) {
            listaAtomos[i].posicion = posicionesFinales[i];
        }
        return true;
    }

    return false;
}

