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
    Q_UNUSED(valores);
    if (stream.atEnd() || listaAtomos.isEmpty()) return false;

    QString primeraLinea = stream.readLine().trimmed();
    int numAtomsCheck = primeraLinea.toInt();
    if (numAtomsCheck != listaAtomos.size()) return false;

    stream.readLine(); // Saltar comentario

    QVector<QVector3D> posicionesFinales;
    const float PIXELES_POR_ANGSTROM = 100.0f;

    while (!stream.atEnd()) {
        QString linea = stream.readLine().trimmed();
        if (linea.isEmpty()) continue;

        QStringList columnas = linea.split(' ', Qt::SkipEmptyParts);
        // ORCA XYZ estándar: [0]=Símbolo, [1]=X, [2]=Y, [3]=Z
        if (columnas.size() >= 4) {
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

    if (posicionesFinales.size() == listaAtomos.size()) {
        for (int i = 0; i < listaAtomos.size(); ++i) {
            listaAtomos[i].posicion = posicionesFinales[i];
        }
        return true;
    }

    return false;
}

bool Datos::leerOrcaPropertySummary(
    QTextStream& stream,
    ValoresCalculo& valores
    ){
    QString seccionActual = "";
    bool exito = false;

    // Variables de control para leer el vector del dipolo línea a línea de forma segura
    bool leyendoDipoloTotal = false;
    int componenteDipoloContador = 0;
    double dx = 0.0, dy = 0.0, dz = 0.0;
    bool okX = false, okY = false, okZ = false;

    while (!stream.atEnd()) {
        QString linea = stream.readLine().trimmed();
        if (linea.isEmpty()) continue;

        // Detectar los bloques delimitadores del property.txt de ORCA
        if (linea.startsWith("$")) {
            seccionActual = linea;
            leyendoDipoloTotal = false; // Reset de seguridad al cambiar de sección
            continue;
        }

        QStringList tokens = linea.split(' ', Qt::SkipEmptyParts);
        if (tokens.isEmpty()) continue;

        // =====================================================================
        // 1. ENERGÍA TOTAL ABSOLUTA (CORREGIDO: Filtro estricto anti-tablas)
        // =====================================================================
        if (seccionActual == "$SCF_Energy" || seccionActual == "$Single_Point_Data") {
            // Captura la línea con '&FinalEnergy' o la fila de datos pura de la energía (0 y el valor)
            if (linea.contains("FinalEnergy") || (tokens.first() == "0" && tokens.size() == 2)) {
                bool ok;
                double eVale = tokens.last().toDouble(&ok);
                if (ok && eVale < -1.0) { // Salvaguarda: las energías moleculares totales siempre son muy negativas
                    valores.energia = QString("%1 u.a.").arg(eVale, 0, 'f', 6);
                    valores.calorFormacion = QString("%1 kcal/mol (Etot)").arg(eVale * 627.5093, 0, 'f', 2);
                    exito = true;
                }
            }
        }
        // =====================================================================
        // 2. MOMENTO DIPOLAR VECTORIAL (LÍNEA A LÍNEA, SIN CRASHES)
        // =====================================================================
        else if (seccionActual == "$SCF_Dipole_Moment") {
            if (linea.startsWith("&dipoleTotal")) {
                leyendoDipoloTotal = true;
                componenteDipoloContador = 0;
                continue;
            }

            if (leyendoDipoloTotal) {
                bool okConversion = false;
                double valorComponente = tokens.last().toDouble(&okConversion);

                if (okConversion) {
                    if (componenteDipoloContador == 0) { dx = valorComponente; okX = true; }
                    else if (componenteDipoloContador == 1) { dy = valorComponente; okY = true; }
                    else if (componenteDipoloContador == 2) { dz = valorComponente; okZ = true; }
                }

                componenteDipoloContador++;

                // Cuando ya tenemos los 3 componentes (X, Y, Z), calculamos el vector total
                if (componenteDipoloContador >= 3) {
                    leyendoDipoloTotal = false; // Cerramos el estado

                    if (okX && okY && okZ) {
                        double magnitudAu = qSqrt(dx*dx + dy*dy + dz*dz);
                        double dipoloDebye = magnitudAu * 2.541746; // 1 a.u. = 2.541746 D

                        if (dipoloDebye < 0.001) dipoloDebye = 0.0; // Umbral de simetría

                        valores.dipolo = QString("%1 D").arg(dipoloDebye, 0, 'f', 4);
                    }
                }
            }
        }
        // =====================================================================
        // 3. TIEMPO DE CÓMPUTO CPU
        // =====================================================================
        else if (seccionActual == "$Calculation_Timings") {
            if (linea.contains("SUM")) {
                bool ok;
                float tiempo = tokens.last().toFloat(&ok);
                if (ok) {
                    valores.tiempoCpu = QString("%1 s").arg(tiempo, 0, 'f', 3);
                }
            }
        }
    }
    return exito;
}

bool Datos::leerOrcaTrayectoria(
    QTextStream& stream,
    QVector<Trayectoria>& pelicula,
    QVector<float>& calores,
    const QVector<Atomo>& listaAtomosBase
    ){
    pelicula.clear();
    calores.clear();
    if (listaAtomosBase.isEmpty()) return false;

    int numAtomos = listaAtomosBase.size();
    const float PIXELES_POR_ANGSTROM = 100.0f; // Tu escala canónica de renderizado

    // Nos aseguramos de posicionar el puntero al inicio del stream
    stream.device()->seek(0);

    while (!stream.atEnd()) {
        QString primeraLinea = stream.readLine().trimmed();
        if (primeraLinea.isEmpty()) continue;

        // 1. Validar el número de átomos del fotograma
        int numAtomsCheck = primeraLinea.toInt();
        if (numAtomsCheck != numAtomos) return false;

        // 2. Extraer la energía de la línea de comentario
        // Ejemplo: "Coordinates from ORCA-job /tmp/co2 E -186.080340974860"
        QString segundaLinea = stream.readLine().trimmed();
        ValoresCalculo valoresFotograma;
        float calorKcal = 0.0f;

        if (segundaLinea.contains(" E ")) {
            QString energiaStr = segundaLinea.section(" E ", 1).trimmed();
            bool ok;
            double energiaHartree = energiaStr.toDouble(&ok);
            if (ok) {
                valoresFotograma.energia = QString("%1 u.a.").arg(energiaHartree, 0, 'f', 6);
                calorKcal = static_cast<float>(energiaHartree * 627.5093);
                valoresFotograma.calorFormacion = QString("%1 kcal/mol (Etot)").arg(calorKcal, 0, 'f', 2);

                // Guardamos el calor en el histórico para el perfil de la gráfica inferior
                calores.append(calorKcal);
            }
        }

        // 3. Procesar los átomos correspondientes a este fotograma
        Trayectoria fotograma;
        bool bloqueLeidoOk = true;

        for (int i = 0; i < numAtomos; ++i) {
            if (stream.atEnd()) { bloqueLeidoOk = false; break; }
            QString lineaAtomo = stream.readLine().trimmed();
            QStringList columnas = lineaAtomo.split(' ', Qt::SkipEmptyParts);

            if (columnas.size() >= 4) {
                bool okX, okY, okZ;
                float x = columnas[1].toFloat(&okX) * PIXELES_POR_ANGSTROM;
                float y = columnas[2].toFloat(&okY) * PIXELES_POR_ANGSTROM;
                float z = columnas[3].toFloat(&okZ) * PIXELES_POR_ANGSTROM;

                if (okX && okY && okZ) {
                    Atomo at;
                    at.id = listaAtomosBase[i].id;
                    at.simbolo = columnas[0]; // Mantiene el símbolo del .xyz
                    at.posicion = QVector3D(x, y, z);
                    fotograma.listaAtomos.append(at);
                } else {
                    bloqueLeidoOk = false;
                }
            } else {
                bloqueLeidoOk = false;
            }
        }

        if (bloqueLeidoOk && fotograma.listaAtomos.size() == numAtomos) {
            // Inicializar metadatos por defecto para los QLabels de este frame
            valoresFotograma.ionizacion = "-- eV";
            valoresFotograma.simetria   = "--";
            valoresFotograma.dipolo     = "-- D";
            valoresFotograma.tiempoCpu  = "-- s";

            fotograma.valoresCalculo = valoresFotograma;
            pelicula.append(fotograma);
        }
    }

    return !pelicula.isEmpty();
}

bool Datos::leerOrcaHessFrecuencias(
    QTextStream& streamHess,
    const QVector<Atomo>& listaAtomos,
    QVector<ModoVibracional>& m_modosVibracionales
    ){
    m_modosVibracionales.clear();
    if (listaAtomos.isEmpty()) return false;

    // Volcar el stream completo para realizar saltos por secciones estructurales del .hess
    streamHess.device()->seek(0);
    QString todoElTexto = streamHess.readAll();

    // =========================================================================
    // FASE 1: LEER FRECUENCIAS ABSOLUTAS ($vibrational_frequencies)
    // =========================================================================
    int idxFreqs = todoElTexto.indexOf("$vibrational_frequencies");
    if (idxFreqs == -1) return false;

    QString seccionFreqs = todoElTexto.mid(idxFreqs);
    QTextStream lectorFreqs(&seccionFreqs);
    lectorFreqs.readLine(); // Consumir cabecera "$vibrational_frequencies"

    int totalModosTotales = lectorFreqs.readLine().trimmed().toInt(); // Número de modos totales calculados
    QVector<float> frecuenciasTemporales;

    for (int i = 0; i < totalModosTotales; ++i) {
        if (lectorFreqs.atEnd()) break;
        QString linea = lectorFreqs.readLine().trimmed();
        QStringList tokens = linea.split(' ', Qt::SkipEmptyParts);
        if (tokens.size() >= 2) {
            frecuenciasTemporales.append(tokens[1].toFloat());
        }
    }

    // =========================================================================
    // FASE 2: LEER INTENSIDADES DEL ESPECTRO IR ($ir_spectrum)
    // =========================================================================
    int idxIr = todoElTexto.indexOf("$ir_spectrum");
    if (idxIr == -1) return false;

    QString seccionIr = todoElTexto.mid(idxIr);
    QTextStream lectorIr(&seccionIr);
    lectorIr.readLine(); // Consumir cabecera "$ir_spectrum"
    lectorIr.readLine(); // Consumir conteo redundante

    QVector<float> intensidadesTemporales;
    for (int i = 0; i < totalModosTotales; ++i) {
        if (lectorIr.atEnd()) break;
        QString linea = lectorIr.readLine().trimmed();
        QStringList tokens = linea.split(' ', Qt::SkipEmptyParts);
        // El formato de ORCA es: [0]=wavenumber, [1]=eps, [2]=Intensidad_IR, [3..5]=TX,TY,TZ
        if (tokens.size() >= 3) {
            intensidadesTemporales.append(tokens[2].toFloat());
        }
    }

    // =========================================================================
    // FASE 3: LEER MATRIZ DE MODOS NORMALES ($normal_modes)
    // =========================================================================
    int idxModos = todoElTexto.indexOf("$normal_modes");
    if (idxModos == -1) return false;

    QString seccionModos = todoElTexto.mid(idxModos);
    QTextStream lectorModos(&seccionModos);
    lectorModos.readLine(); // Consumir cabecera "$normal_modes"

    QStringList dimensiones = lectorModos.readLine().trimmed().split(' ', Qt::SkipEmptyParts);
    if (dimensiones.size() < 2) return false;

    int filasMatriz = dimensiones[0].toInt(); // Coordenadas Cartesianas (3 * N_átomos)
    int columnasMatriz = dimensiones[1].toInt(); // Modos vibracionales totales

    // Inicializar mapas dinámicos para reconstruir los bloques verticales del .hess
    QMap<int, QVector<QVector3D>> mapaDesplazamientos;
    for (int col = 0; col < columnasMatriz; ++col) {
        mapaDesplazamientos[col].resize(listaAtomos.size());
    }

    // Factor de conversión canónico de tu visor para transformar la deformación a píxeles
    const float PIXELES_POR_ANGSTROM = 100.0f;

    // Procesar la matriz organizada en bloques de columnas horizontales
    while (!lectorModos.atEnd()) {
        QString linea = lectorModos.readLine().trimmed();

        // Si salimos de la matriz o llegamos a una nueva sección, rompemos el bucle
        if (linea.startsWith("$") && !linea.startsWith("$normal_modes")) {
            break;
        }
        if (linea.isEmpty() || linea.startsWith("#")) {
            continue;
        }

        QStringList columnasHeaders = linea.split(' ', Qt::SkipEmptyParts);

        // Verificamos si la línea es una cabecera de bloque (contiene los índices de los modos, ej: 0 1 2 3 4)
        if (!columnasHeaders.isEmpty() && !columnasHeaders[0].contains(".") && columnasHeaders[0].toInt() >= 0) {
            QVector<int> indicesColumnasBloque;
            for (const QString& h : columnasHeaders) {
                indicesColumnasBloque.append(h.toInt());
            }

            // Leer las 'filasMatriz' consecutivas asociadas a este bloque horizontal de columnas
            for (int r = 0; r < filasMatriz; ++r) {
                if (lectorModos.atEnd()) break;

                QString lineaFila = lectorModos.readLine().trimmed();
                QStringList tokensFila = lineaFila.split(' ', Qt::SkipEmptyParts);
                if (tokensFila.size() < 2) continue;

                int idFilaMatriz = tokensFila[0].toInt(); // Índice de la fila cartesiana (0, 1, 2...)

                // 🎯 MApEADO BIUNÍVOCO CORRECTO PARA ORCA:
                int idAtomoMapeado = idFilaMatriz / 3;
                int ejeActivo      = idFilaMatriz % 3;

                if (idAtomoMapeado >= listaAtomos.size()) continue;

                // Recorremos los valores de desplazamiento del token correspondientes a cada columna del bloque
                for (int c = 1; c < tokensFila.size(); ++c) {
                    if ((c - 1) >= indicesColumnasBloque.size()) break;

                    int columnaGlobal = indicesColumnasBloque[c - 1];
                    if (columnaGlobal >= columnasMatriz) continue;

                    float valorDesplazamiento = tokensFila[c].toFloat();
                    float valorEscaladoPixel = valorDesplazamiento * PIXELES_POR_ANGSTROM;

                    // Inyección segura de componentes cartesianos en base al módulo 3 del índice ORCA
                    if (ejeActivo == 0) {
                        mapaDesplazamientos[columnaGlobal][idAtomoMapeado].setX(valorEscaladoPixel);
                    } else if (ejeActivo == 1) {
                        mapaDesplazamientos[columnaGlobal][idAtomoMapeado].setY(valorEscaladoPixel);
                    } else if (ejeActivo == 2) {
                        mapaDesplazamientos[columnaGlobal][idAtomoMapeado].setZ(valorEscaladoPixel);
                    }
                }
            }
        }
    }

    // =========================================================================
    // FASE 4: ENSAMBLAJE DE MODOS FILTRANDO ROTACIONES/TRASLACIONES (Freq > 20 cm⁻¹)
    // =========================================================================
    for (int i = 0; i < frecuenciasTemporales.size(); ++i) {
        float valorCm1 = frecuenciasTemporales[i];

        // Filtro cuántico estricto: Eliminamos los primeros 6 modos residuales (valores cercanos a 0.0)
        if (valorCm1 < 20.0f && valorCm1 > -20.0f) {
            continue;
        }

        ModoVibracional modo;
        modo.numero = m_modosVibracionales.size() + 1; // ID auto-incremental correlativo para la UI
        modo.frecuencia = valorCm1;
        modo.intensidad = (i < intensidadesTemporales.size()) ? intensidadesTemporales[i] : 0.0f;
        modo.desplazamientos = mapaDesplazamientos[i];

        m_modosVibracionales.append(modo);
    }

    return !m_modosVibracionales.isEmpty();
}