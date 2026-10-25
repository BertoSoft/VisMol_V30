#include "grafica.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QString>

Grafica::Grafica(QWidget *parent)
    : QWidget(parent)
    , m_pasoActual(0)
    , m_minY(0.0)
    , m_maxY(0.0)
    , m_hayFrecuencias(false){
    // Permitir la expansión elástica del widget dentro de layouts en Qt Designer
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void Grafica::setDatosCalores(const QVector<float>& calores) {
    m_valoresEnergia.clear();

    // Convertimos los floats leídos del .out a doubles para reutilizar el motor de dibujo existente
    for (float valor : calores) {
        m_valoresEnergia.append(static_cast<double>(valor));
    }

    if (m_valoresEnergia.isEmpty()) return;

    // Búsqueda automática de los extremos del histórico de optimización estática
    m_minY = m_valoresEnergia[0];
    m_maxY = m_valoresEnergia[0];
    for (double e : m_valoresEnergia) {
        if (e < m_minY) m_minY = e;
        if (e > m_maxY) m_maxY = e;
    }

    // Amortiguación visual del 15% para mantener la estética limpia del marco físico
    double margen = (m_maxY - m_minY) * 0.15;
    if (margen == 0.0) margen = 10.0; // Salvaguarda anti-división por cero
    m_minY -= margen;
    m_maxY += margen;

    update(); // Forzar repintado síncrono inmediato en el bucle de Qt
}

void Grafica::setDatos(const QVector<Trayectoria>& pelicula) {
    m_valoresEnergia.clear();

    // Extracción limpia de la magnitud numérica aislando el token de texto "kcal/mol"
    for (const Trayectoria& fotograma : pelicula) {
        QString numeroStr = fotograma.valoresCalculo.calorFormacion.section(' ', 0, 0);
        bool ok;
        double valor = numeroStr.toDouble(&ok);
        if (ok) {
            m_valoresEnergia.append(valor);
        }
    }

    if (m_valoresEnergia.isEmpty()) return;

    // Búsqueda automática de los extremos del histórico de optimización
    m_minY = m_valoresEnergia[0];
    m_maxY = m_valoresEnergia[0];
    for (double e : m_valoresEnergia) {
        if (e < m_minY) m_minY = e;
        if (e > m_maxY) m_maxY = e;
    }

    // Amortiguación visual del 15% para que la curva nunca colisione con el marco físico
    double margen = (m_maxY - m_minY) * 0.15;
    if (margen == 0.0) margen = 10.0; // Salvaguarda anti-división por cero
    m_minY -= margen;
    m_maxY += margen;

    update(); // Forzar repintado inmediato en el bucle de Qt
}

void Grafica::setDatosEspectro(const QVector<ModoVibracional>& modos){
    m_valoresEnergia.clear();
    m_frecuenciasX.clear();
    m_intensidadesY.clear();
    m_numerosModoReal.clear(); // 🎯 Limpieza preventiva

    m_hayFrecuencias = !modos.isEmpty();

    if (!m_hayFrecuencias) {
        update();
        return;
    }

    for (const ModoVibracional& modo : modos) {
        if (modo.intensidad >= 0.0f) {
            m_frecuenciasX.append(modo.frecuencia);
            m_intensidadesY.append(modo.intensidad);

            // 🎯 SINCRO: Guardamos el número del modo real (ej: 1, 2, 19...)
            m_numerosModoReal.append(modo.numero);
        }
    }

    if (m_frecuenciasX.isEmpty()) {
        m_hayFrecuencias = false;
        update();
        return;
    }

    // [Tu búsqueda de m_minY, m_maxY, minX, maxX se mantiene exactamente idéntica]
    m_minY = 0.0;
    m_maxY = static_cast<double>(m_intensidadesY[0]);
    m_minX_Espectro = static_cast<double>(m_frecuenciasX[0]);
    m_maxX_Espectro = static_cast<double>(m_frecuenciasX[0]);

    for (int i = 0; i < m_frecuenciasX.size(); ++i) {
        if (static_cast<double>(m_intensidadesY[i]) > m_maxY) m_maxY = static_cast<double>(m_intensidadesY[i]);
        if (static_cast<double>(m_frecuenciasX[i]) < m_minX_Espectro) m_minX_Espectro = static_cast<double>(m_frecuenciasX[i]);
        if (static_cast<double>(m_frecuenciasX[i]) > m_maxX_Espectro) m_maxX_Espectro = static_cast<double>(m_frecuenciasX[i]);
    }

    if (m_maxY == 0.0) m_maxY = 10.0;
    m_maxY *= 1.15;

    if (m_maxX_Espectro - m_minX_Espectro < 200.0) {
        m_minX_Espectro -= 100.0; m_maxX_Espectro += 100.0;
    } else {
        double margenX = (m_maxX_Espectro - m_minX_Espectro) * 0.05;
        m_minX_Espectro -= margenX; m_maxX_Espectro += margenX;
    }

    update();
}

void Grafica::setPasoActual(int paso) {
    m_pasoActual = paso;
    update(); // Redibuja el marcador móvil de fase
}

void Grafica::setModoActivo(int idxModo){
    m_idxModoActivoGrafica = idxModo;
    update();
}

void Grafica::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Fondo adaptativo de terminal (Activo vs Inactivo)
    if (this->isEnabled()) {
        painter.fillRect(rect(), QColor("#030712"));
    } else {
        painter.fillRect(rect(), QColor("#2e2e2e"));
    }

    // Salvaguardas analíticas según el contexto activo
    if (!m_hayFrecuencias && m_valoresEnergia.size() < 2) return;
    if (m_hayFrecuencias && m_frecuenciasX.isEmpty()) return;

    int padIzdo = 65;
    int padDcho = 20;
    int padSup  = 20;
    int padInf  = 35;

    int graphWidth  = width() - padIzdo - padDcho;
    int graphHeight = height() - padSup - padInf;

    // 1. DIBUJO DE LOS EJES COORDENADOS DE RESPALDO
    painter.setPen(QPen(QColor("#334155"), 1));
    painter.drawLine(padIzdo, padSup, padIzdo, height() - padInf); // Eje Y
    painter.drawLine(padIzdo, height() - padInf, width() - padDcho, height() - padInf); // Eje X

    painter.setPen(QColor("#94A3B8"));
    painter.setFont(QFont("Consolas", 8));

    // =========================================================================
    // CASO A: CONTEXTO ESPECTROSCÓPICO ABSORCIÓN INFRARROJA (IR)
    // =========================================================================
    if (m_hayFrecuencias) {
        // Rotulación estática de extremos en el eje Y (Intensidad)
        painter.drawText(5, padSup + 5, padIzdo - 10, 20, Qt::AlignRight, QString::number(m_maxY, 'f', 1));
        painter.drawText(5, height() - padInf - 15, padIzdo - 10, 20, Qt::AlignRight, "0.0");

        // Rotulación estática de extremos en el eje X (Frecuencias)
        painter.drawText(padIzdo, height() - padInf + 5, 90, 20, Qt::AlignLeft, QString("%1 cm⁻¹").arg(m_minX_Espectro, 0, 'f', 0));
        painter.drawText(width() - padDcho - 90, height() - padInf + 5, 90, 20, Qt::AlignRight, QString("%1 cm⁻¹").arg(m_maxX_Espectro, 0, 'f', 0));

        // Rótulo central de magnitud inferior
        painter.setFont(QFont("Consolas", 9, QFont::Bold));
        painter.drawText(padIzdo, height() - padInf + 18, graphWidth, 20, Qt::AlignCenter, tr("Frecuencia de Onda (cm⁻¹)"));

        // RECORRIDO Y TRAZADO INTERNO DE SEÑALES ESPECTRALES
        for (int i = 0; i < m_frecuenciasX.size(); ++i) {
            double freq  = static_cast<double>(m_frecuenciasX[i]);
            double intens = static_cast<double>(m_intensidadesY[i]);

            double xPixel = padIzdo + ((freq - m_minX_Espectro) / (m_maxX_Espectro - m_minX_Espectro)) * graphWidth;
            double yPixel = height() - padInf - (intens / m_maxY) * graphHeight;
            double baseEjeX = height() - padInf;

            // 🎯 LA SOLUCCIÓN HISTÓRICA: Comparamos el número de modo físico de la RAM
            // Buscamos si el número de modo de este pico coincide con el seleccionado en la UI
            if (m_numerosModoReal[i] == m_idxModoActivoGrafica) {
                // Modo activo real: Se ilumina en un Rojo intenso impecable
                painter.setPen(QPen(QColor("#EF4444"), 3, Qt::SolidLine, Qt::RoundCap));
                painter.drawLine(QPointF(xPixel, baseEjeX), QPointF(xPixel, yPixel));
            }
            else {
                // Modos normales: se quedan en el Verde Esmeralda por defecto
                painter.setPen(QPen(QColor("#10B981"), 2, Qt::SolidLine, Qt::RoundCap));
                painter.drawLine(QPointF(xPixel, baseEjeX), QPointF(xPixel, yPixel));
            }
        }
    }
    // =========================================================================
    // CASO B: CONTEXTO CONVENCIONAL DE CONVERGENCIA ENERGÉTICA
    // =========================================================================
    else {
        auto mapearPunto = [&](int indice) -> QPointF {
            double x = padIzdo + ((double)indice / (m_valoresEnergia.size() - 1)) * graphWidth;
            double razonY = (m_valoresEnergia[indice] - m_minY) / (m_maxY - m_minY);
            return QPointF(x, height() - padInf - (razonY * graphHeight));
        };

        QPainterPath rutaCurva;
        rutaCurva.moveTo(mapearPunto(0));
        for (int i = 1; i < m_valoresEnergia.size(); ++i) {
            rutaCurva.lineTo(mapearPunto(i));
        }
        painter.setPen(QPen(QColor("#10B981"), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(rutaCurva);

        painter.drawText(5, padSup + 5, padIzdo - 10, 20, Qt::AlignRight, QString::number(m_maxY, 'f', 1));
        painter.drawText(5, height() - padInf - 15, padIzdo - 10, 20, Qt::AlignRight, QString::number(m_minY, 'f', 1));
        painter.setFont(QFont("Consolas", 9, QFont::Bold));
        painter.drawText(padIzdo, height() - padInf + 8, graphWidth, 20, Qt::AlignCenter,
                         QString(tr("Etapa de Optimización (Total: %1)")).arg(m_valoresEnergia.size()));

        if (m_pasoActual >= 0 && m_pasoActual < m_valoresEnergia.size()) {
            QPointF puntoCurva = mapearPunto(m_pasoActual);
            painter.setPen(QPen(QColor("#38BDF8"), 1, Qt::DashLine));
            painter.drawLine(puntoCurva.x(), puntoCurva.y(), puntoCurva.x(), height() - padInf);
            painter.setPen(QPen(QColor("#38BDF8"), 2));
            painter.setBrush(QColor("#0F172A"));
            painter.drawEllipse(puntoCurva, 4, 4);
        }
    }
}