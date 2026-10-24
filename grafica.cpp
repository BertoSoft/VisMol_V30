#include "grafica.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QString>

Grafica::Grafica(QWidget *parent)
    : QWidget(parent)
    , m_pasoActual(0)
    , m_minY(0.0)
    , m_maxY(0.0) {
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

void Grafica::setPasoActual(int paso) {
    m_pasoActual = paso;
    update(); // Redibuja el marcador móvil de fase
}

void Grafica::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 🎯 REQUISITO: Cambiar el fondo dinámicamente si el widget está activo o inactivo
    if (this->isEnabled()) {
        painter.fillRect(rect(), QColor("#030712")); // Fondo negro de terminal activo
    } else {
        painter.fillRect(rect(), QColor("#2e2e2e")); // Mismo fondo que barraDerecha cuando está inactivo
    }

    if (m_valoresEnergia.size() < 2) {
        // Solo pintamos el texto de aviso si el widget está habilitado para no ensuciar el fondo
        return;
    }

    // Márgenes estáticos de la rejilla gráfica (Padding)
    int padIzdo = 65;
    int padDcho = 20;
    int padSup  = 20;
    int padInf  = 35;

    int graphWidth  = width() - padIzdo - padDcho;
    int graphHeight = height() - padSup - padInf;

    // 1. DIBUJO DE LOS EJES COORDENADOS (Gris pizarra elegante)
    painter.setPen(QPen(QColor("#334155"), 1));
    // Eje Y
    painter.drawLine(padIzdo, padSup, padIzdo, height() - padInf);
    // Eje X
    painter.drawLine(padIzdo, height() - padInf, width() - padDcho, height() - padInf);

    // Lambda matemática para transformar (Índice, Calor) -> Coordenada píxel (X, Y)
    auto mapearPunto = [&](int indice) -> QPointF {
        double x = padIzdo + ((double)indice / (m_valoresEnergia.size() - 1)) * graphWidth;
        double razonY = (m_valoresEnergia[indice] - m_minY) / (m_maxY - m_minY);
        // Inversión implícita de coordenadas porque el origen (0,0) de Qt nace arriba a la izquierda
        double y = height() - padInf - (razonY * graphHeight);
        return QPointF(x, y);
    };

    // 2. TRAZADO CONTINUO DE LA CURVA ENERGÉTICA
    QPainterPath rutaCurva;
    rutaCurva.moveTo(mapearPunto(0));
    for (int i = 1; i < m_valoresEnergia.size(); ++i) {
        rutaCurva.lineTo(mapearPunto(i));
    }
    // Color Verde Terminal Brillante idéntico a tus labels de resultados (#10B981)
    painter.setPen(QPen(QColor("#10B981"), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(rutaCurva);

    // 3. ROTULACIÓN Y TEXTO DE EJES (Tipografía Consolas)
    painter.setPen(QColor("#94A3B8"));
    painter.setFont(QFont("Consolas", 8));

    // Valores extremos en el Eje Y
    painter.drawText(5, padSup + 5, padIzdo - 10, 20, Qt::AlignRight, QString::number(m_maxY, 'f', 1));
    painter.drawText(5, height() - padInf - 15, padIzdo - 10, 20, Qt::AlignRight, QString::number(m_minY, 'f', 1));

    // Texto central indicativo en el Eje X
    painter.setFont(QFont("Consolas", 9, QFont::Bold));
    painter.drawText(padIzdo, height() - padInf + 8, graphWidth, 20, Qt::AlignCenter,
                     QString(tr("Etapa de Optimización (Total: %1)")).arg(m_valoresEnergia.size()));

    // 4. INDICADOR DE FASE ACTUAL (Tu propuesta: línea vertical desde el punto hasta el suelo)
    if (m_pasoActual >= 0 && m_pasoActual < m_valoresEnergia.size()) {
        QPointF puntoCurva = mapearPunto(m_pasoActual);
        double baseEjeX = height() - padInf; // Coordenada Y de la base del Eje X

        // Línea vertical recta discontinua en Azul Cian complementario (#38BDF8)
        painter.setPen(QPen(QColor("#38BDF8"), 1, Qt::DashLine));
        painter.drawLine(puntoCurva.x(), puntoCurva.y(), puntoCurva.x(), baseEjeX);

        // Nodo circular destacado en la intersección de la curva
        painter.setPen(QPen(QColor("#38BDF8"), 2));
        painter.setBrush(QColor("#0F172A")); // Fondo oscuro saturado para contraste de máscara
        painter.drawEllipse(puntoCurva, 4, 4);
    }
}