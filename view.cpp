#include "view.h"
#include "atomos.h"
#include "datos.h"

#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QDir>

View::View(QObject *parent)
    : QObject(parent){

}

View::~View(){

}


// =========================================================================
// FUNCIONES PUBLICAS
// =========================================================================
void View::initUi(){
    resetValores();
    m_estado.estaEmpezado = false;

    emit actualizarVista(m_estado);
}

void View::setClick(const QVector3D& posClick){
    switch (m_estado.modoEditor) {
    case ModoSeleccion:
        procesarClickSeleccion(posClick);
        break;
    case ModoDibujo:
        procesarClickDibujo(posClick);
        break;
    case ModoCrearEnlace:
        procesarClickCrearEnlace(posClick);
        break;
    case ModoRotacion3D:
        procesarClickRotacion(posClick);
        break;
    default:
        return;
    }
}

void View::setClickDerecho(const QVector3D& posClick){

    // 1. Buscamos primero si hay un átomo bajo el cursor
    int idAtomo     = getIdAtomoFromPosicion(posClick);
    int idEnlace    = -1; // De momento mientras no medimos distancias

    // 2. Si NO hay ningún átomo, buscamos si el usuario ha pulsado sobre una línea (enlace)
    if (idAtomo < 0) {
        const float TOLERANCIA_CLIC_ENLACE = 12.0f; // Margen de error en píxeles/unidades para acertar a la línea
        float distanciaMinima = TOLERANCIA_CLIC_ENLACE;

        for (int i = 0; i < m_listaEnlaces.size(); ++i) {
            const Enlace& enlace = m_listaEnlaces[i];

            // Recuperamos las coordenadas de los dos átomos que forman el enlace
            QVector3D pos1, pos2;
            bool e1 = false, e2 = false;
            for (const Atomo& a : m_listaAtomos) {
                if (a.id == enlace.id_atomo1) { pos1 = a.posicion; e1 = true; }
                if (a.id == enlace.id_atomo2) { pos2 = a.posicion; e2 = true; }
                if (e1 && e2) break;
            }

            if (e1 && e2) {
                // Medimos la distancia geométrica desde el clic hasta este segmento de enlace
                float dist = getDistanciaPuntoSegmento(posClick, pos1, pos2);
                if (dist < distanciaMinima) {
                    distanciaMinima = dist;
                    idEnlace = i; // Guardamos el índice del enlace más cercano
                }
            }
        }
    }


    if(idAtomo>=0 || idEnlace>=0){
        emit setMenuEliminar(idAtomo, idEnlace);
    }
}

void View::eliminarAtomo(int idAtomo) {
    if (idAtomo < 0) return;

    // 1. Guardamos una foto en el historial antes de romper nada
    setEstadoHistorial();

    // 2. Eliminamos todos los enlaces asociados a este átomo (por seguridad física/química)
    for (int i = m_listaEnlaces.size() - 1; i >= 0; --i) {
        if (m_listaEnlaces[i].id_atomo1 == idAtomo || m_listaEnlaces[i].id_atomo2 == idAtomo) {
            m_listaEnlaces.removeAt(i);
        }
    }

    // 3. Eliminamos el átomo de la lista
    for (int i = 0; i < m_listaAtomos.size(); ++i) {
        if (m_listaAtomos[i].id == idAtomo) {
            m_listaAtomos.removeAt(i);
            break;
        }
    }

    // 🎯 REPARACIÓN DE LOGICA: Sincronizar el conteo basal con la nueva realidad de la RAM
    // Esto evita que notificaEstado() interprete un cambio destructivo y limpie la selección
    m_listaAtomosTrabajo = m_listaAtomos;
    m_numeroAtomos  = m_listaAtomos.size();
    m_numeroEnlaces = m_listaEnlaces.size();
    m_idAtomoSeleccionado = -1; // Limpieza preventiva del cabezal de enlace

    // 4. Refrescamos todo el sistema
    notificaEstado();
}

void View::eliminarEnlace(int idEnlace) {
    if (idEnlace < 0 || idEnlace >= m_listaEnlaces.size()) return;

    // Guarda la memoria
    setEstadoHistorial();

    // Buscamos el enlace por las identificaciones de sus átomos
    m_listaEnlaces.removeAt(idEnlace);

    // 🎯 REPARACIÓN DE LOGICA: Sincronizar el conteo de enlaces modificados
    m_numeroEnlaces = m_listaEnlaces.size();
    m_idAtomoSeleccionado = -1;

    // Notificamos e invocamos al lienzo para que repinte las líneas inmediatamente
    notificaEstado();
}

void View::nuevoProyecto(){
    limpiarLienzo(); // ya actualiza m_estado
    m_estado.estaEmpezado = true;
    notificaEstado();
}

void View::abrirProyecto(const QString& ruta){
    if (ruta.isEmpty()) return;

    // 1. Limpieza protocolaria antes de cargar nuevos datos
    limpiarLienzo();

    // 2. Llamada delegada a la persistencia pura pasándole las referencias de la RAM
    bool exito = Datos::abrirVisMol(ruta, m_listaAtomos, m_listaEnlaces, m_comentario);

    if (exito) {
        m_listaAtomosTrabajo = m_listaAtomos;

        // 3. Buscar el ID más alto cargado para que el contador incremental no colisione
        int maxIdDetectado = 0;
        for (const Atomo& a : m_listaAtomos) {
            if (a.id > maxIdDetectado) {
                maxIdDetectado = a.id;
            }
        }

        // 4. Sincronizar el estado del motor y de almacenamiento en disco
        m_rutaArchivoActual = ruta;
        m_contadorId        = maxIdDetectado;
        m_numeroAtomos      = m_listaAtomos.size();  // Apaga el asterisco '*' de modificación
        m_numeroEnlaces     = m_listaEnlaces.size(); // Apaga el asterisco '*' de modificación

        m_estado.modoEditor         = ModoEditor::ModoSeleccion;
        m_estado.estaEmpezado       = true;
        m_estado.estaModificado     = false;
    } else {
        // Si el archivo falla o está corrupto, nos aseguramos de que el sistema quede cerrado
        m_estado.estaEmpezado       = false;
        m_estado.estaModificado     = false;
    }

    // 5. Notificar de inmediato al Lienzo y a la Vista
    notificaEstado();
}

void View::guardarProyecto(){
    if (m_rutaArchivoActual.isEmpty()) return;

    // Llamamos a DatosIo pasándole los datos puros
    bool exito = Datos::guardarVisMol(m_rutaArchivoActual, m_listaAtomos, m_listaEnlaces, m_comentario);

    if (exito) {
        // Sincronizamos el conteo histórico para apagar el asterisco '*'
        m_numeroAtomos  = m_listaAtomos.size();
        m_numeroEnlaces = m_listaEnlaces.size();

        m_estado.estaModificado = false;
        m_estado.estaGuardado   = true; // ◄ Forzamos el estado guardado con éxito
        notificaEstado();
    }
}

void View::cerrarProyecto(){
    limpiarLienzo();
    m_estado.estaEmpezado = false;
    notificaEstado();
}

void View::lanzarCalculoQm(){
    if (m_listaAtomos.isEmpty()) return;

    // 1. Congelar la interfaz de usuario al arrancar la simulación
    m_estado.hayProceso = true;
    notificaEstado();

    // 2. Control de contingencia básico
    if (m_argumentos.trimmed().isEmpty()) {
        m_argumentos = "PM7 OPT DIPOLE NOINTER";
    }

    // 3. Evaluar la intención original de las keywords
    TipoCalculo tipoCalculoFaseActual = getTipoCalculo();
    QString argumentosEjecucion = m_argumentos;

    // 🎯 MÁQUINA DE ESTADOS SECUENCIAL (FASE 1):
    // Si el usuario pide Frecuencias (FORCE) y la geometría NO está optimizada aún:
    if (tipoCalculoFaseActual == TipoCalculo::MopacFreq && !m_estado.estaOptimizado) {
        // Guardamos la bandera para la fase asíncrona
        this->setProperty("esperandoForceSecuencial", true);

        // Aislamos el método químico del usuario (ej: "PM7") y forzamos el cálculo previo de OPT
        QString metodo = m_argumentos.split(' ', Qt::SkipEmptyParts).first();
        argumentosEjecucion = metodo + " OPT DIPOLE NOINTER";

        // 🌟 CLAVE: Forzamos que esta primera fase sea estrictamente una Optimización
        tipoCalculoFaseActual = TipoCalculo::MopacOpt;
    }

    // 4. Construir la ruta única para el archivo .mop en el directorio temporal
    QString nombreBase = QFileInfo(m_rutaArchivoActual).baseName();
    if (nombreBase.isEmpty()) nombreBase = "mopac_job";
    QString rutaInputMop = QDir::tempPath() + "/" + nombreBase + ".mop";

    // 5. Escritura del archivo de entrada de MOPAC (.mop)
    QFile archivoMop(rutaInputMop);
    if (!archivoMop.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_estado.hayProceso = false;
        notificaEstado();
        return;
    }

    QTextStream out(&archivoMop);
    out << argumentosEjecucion << "\n";
    out << "VisMol Automated Task - " << nombreBase << "\n\n";

    // Conversión de escala Angstrom/Píxeles
    const float PIXELES_POR_ANGSTROM = 100.0f;
    for (const Atomo& a : m_listaAtomos) {
        float x = a.posicion.x() / PIXELES_POR_ANGSTROM;
        float y = a.posicion.y() / PIXELES_POR_ANGSTROM;
        float z = a.posicion.z() / PIXELES_POR_ANGSTROM;
        out << a.simbolo << " " << x << " 1 " << y << " 1 " << z << " 1\n";
    }
    archivoMop.close();

    // Backup preventivo de la conectividad (RAM -> Disco temporal)
    QString rutaCopiaVisMol = rutaInputMop.section('.', 0, -2) + ".vismol";
    Datos::guardarVisMol(rutaCopiaVisMol, m_listaAtomos, m_listaEnlaces, m_comentario);

    // 6. Lanzamos el ejecutable asíncrono pasándole el tipo exacto de ESTA fase
    this->lanzarCalculoMopac(rutaInputMop, tipoCalculoFaseActual);
}

Estado View::getEstado(){
    return m_estado;
}

ValoresCalculo View::getValoresCalculo(){
    return m_valoresCalculo;
}

QString View::getAtomoActivo(){
    return m_atomoActivo;
}

QVector<Atomo> View::getListaAtomos(){
    return m_listaAtomosTrabajo;
}

QVector<Enlace> View::getListaEnlaces(){
    return m_listaEnlaces;
}

QString View::getComentario(){
    return m_comentario;   
}

QString View::getRutaActual(){
    return m_rutaArchivoActual;
}

QString View::getArgumentos(){
    return m_argumentos;
}

int View::getIdAtomoSeleccionado(){
    return m_idAtomoSeleccionado;
}

int View::getNumeroFotogramas(){
    return m_pelicula.size();
}

int View::getFotogramaActual(){
    return m_fotogramaActual;
}

QVector<Trayectoria> View::getPelicula() const{
    return m_pelicula;
}

QVector<float> View::getCalores(){
    return m_calores;
}

QVector<ModoVibracional> View::getModosVibracionales() const{
    return m_modosVibracionales;
}

void View::actualizarFaseVibracion(int indiceModo, float faseSenoidal, float escalaVisual) {
    if (indiceModo < 0 || indiceModo >= m_modosVibracionales.size()) {
        m_listaAtomosTrabajo = m_listaAtomos; // Volver al reposo inmutable
        emit actualizarLienzo();
        return;
    }

    const auto& desplazamientosModo = m_modosVibracionales[indiceModo].desplazamientos;

    if (desplazamientosModo.size() == m_listaAtomos.size()) {
        for (int i = 0; i < m_listaAtomos.size(); ++i) {
            // 🎯 ASIGNACIÓN UNITARIA: Forzamos la deformación matemática directamente en la lista de Trabajo
            m_listaAtomosTrabajo[i].posicion = m_listaAtomos[i].posicion +
                                               (desplazamientosModo[i] * faseSenoidal * escalaVisual);
        }

        // Disparamos la señal nativa de Qt para alertar al lienzo de que la geometría ha cambiado
        emit actualizarLienzo();
    }
}


void View::setFotogramaActual(int fotograma){
    if (m_pelicula.isEmpty()) return;

    // Acotamos el índice por seguridad física dentro de los límites de la película
    m_fotogramaActual = qMax(0, qMin(fotograma, m_pelicula.count() - 1));

    // 🎞️ SINCRONIZACIÓN DE LA RAM:
    // Sustituimos los átomos y enlaces de trabajo activos por las fotos del fotograma seleccionado
    m_listaAtomos           = m_pelicula[m_fotogramaActual].listaAtomos;
    m_listaAtomosTrabajo    = m_listaAtomos; // 🎯 CLONAR AQUÍ
    m_listaEnlaces          = m_pelicula[m_fotogramaActual].listaEnlaces;
    m_valoresCalculo        = m_pelicula[m_fotogramaActual].valoresCalculo;

    notificaEstado();
}

void View::fotogramaAdelante(){
    if (m_fotogramaActual < m_pelicula.count() - 1) {
        setFotogramaActual(m_fotogramaActual + 1);
    } else {
        setFotogramaActual(0); // Bucle infinito (Loop): si llega al final, vuelve al principio
    }
}

void View::fotogramaAtras(){
    if (m_fotogramaActual > 0) {
        setFotogramaActual(m_fotogramaActual - 1);
    } else {
        setFotogramaActual(m_pelicula.count() - 1); // Si retrocede en el inicio, va al último frame
    }
}

void View::setElementoActivo(const QString& simbolo){
    if(simbolo == "") return;

    if(m_estado.modoEditor != ModoEditor::ModoDibujo){
        m_estado.modoEditor = ModoEditor::ModoDibujo;
    }

    m_atomoActivo = simbolo;

    notificaEstado();
}

void View::setModoEditor(ModoEditor modo){
    if(modo == m_estado.modoEditor) return;

    m_estado.modoEditor = modo;

    notificaEstado();
}

void View::setComentario(const QString& comentario){
    m_comentario = comentario;

    m_estado.estaModificado = true;
    notificaEstado();
}

void View::setRutaActual(const QString& ruta){
    m_rutaArchivoActual = ruta;
}

void View::setArgumentos(const QString& argumentos){
    m_argumentos = argumentos;

    notificaEstado();
}

void View::deshacer(){
    atrasHistorial();
}

void View::rehacer(){
    adelanteHistorial();
}

// =========================================================================
// FUNCIONES PRIVADAS
// =========================================================================

void View::resetValores(){

    // 1.- listas de datos
    m_listaAtomos.clear();
    m_listaAtomosTrabajo.clear();
    m_listaEnlaces.clear();
    m_modosVibracionales.clear();
    m_calores.clear();

    // 2.- Valores de View
    m_atomoActivo         = "";
    m_idAtomoSeleccionado = -1;
    m_contadorId          = 0;
    m_rutaArchivoActual   = "";
    m_comentario          = "";
    m_argumentos          = "PM7 OPT DIPOLE NOINTER";
    m_ultimaPosRat        = QVector3D(-10000.0f, -10000.0f, -10000.0f);

    // 2.5 🎯 INICIALIZACIÓN DE VALORES DE CÁLCULO ASOCIADOS A LA INTERFAZ
    m_valoresCalculo.energia        = "-- u.a.";
    m_valoresCalculo.calorFormacion = "-- kcal/mol";
    m_valoresCalculo.ionizacion     = "-- eV";
    m_valoresCalculo.simetria       = "--";
    m_valoresCalculo.dipolo         = "-- D";
    m_valoresCalculo.tiempoCpu      = "-- s";



    // 3.- Reiniciamos las variables de control histórico de disco
    m_numeroAtomos        = 0;
    m_numeroEnlaces       = 0;

    // 4.- Estado base inicial por defecto (Inactivo)
    m_estado.modoEditor     = ModoEditor::ModoSeleccion;
    m_estado.estaVacio      = true;
    m_estado.estaGuardado   = false;
    m_estado.estaModificado = false;
    m_estado.hayProceso     = false;
    m_estado.estaOptimizado = false;
    m_estado.hayDeshacer    = false;
    m_estado.hayRehacer     = false;
    m_estado.hayTrayectoria = false;
    m_estado.hayFrecuencias = false;

    // 5.- Limpieza total del historial físico
    m_historialDeshacer.clear();
    m_historialRehacer.clear();
}

void View::notificaEstado(){
    // 1. Detección automática de vacío
    m_estado.estaVacio = m_listaAtomos.isEmpty() && m_listaEnlaces.isEmpty();

    // 2. Control de historial para los botones de la UI
    m_estado.hayDeshacer = !m_historialDeshacer.isEmpty();
    m_estado.hayRehacer  = !m_historialRehacer.isEmpty();

    // 3. Detección automática de modificación (Comparación con el historial guardado)
    if (!m_estado.estaEmpezado) {
        m_estado.estaModificado = false;
        m_estado.estaGuardado   = false;
    }
    else {
        // Un proyecto está modificado si el conteo actual difiere del guardado
        bool haCambiadoContenido = (m_listaAtomos.size() != m_numeroAtomos) ||
                                   (m_listaEnlaces.size() != m_numeroEnlaces) ||
                                   m_estado.estaModificado;

        m_estado.estaModificado = haCambiadoContenido;

        if (haCambiadoContenido) {
            m_estado.estaOptimizado = false;
            m_estado.hayTrayectoria = false; // 🎯 SEGURIDAD: Si se altera la RAM, la trayectoria se destruye
            m_pelicula.clear();              // Vaciamos la película vieja para liberar RAM
            m_fotogramaActual = 0;
        }

        // Está guardado si tiene una ruta de disco asignada Y además no ha sido modificado
        m_estado.estaGuardado   = (!m_rutaArchivoActual.isEmpty()) && !m_estado.estaModificado;
    }

    // 3. Emitir las señales hacia el Lienzo y la Vista
    emit actualizarLienzo();
    emit actualizarVista(m_estado);
}

void View::limpiarLienzo(){

    resetValores();
    notificaEstado();
}

int View::getIdAtomoFromPosicion(const QVector3D& posClick){

    for(const Atomo& atomo: m_listaAtomos){
        float distancia = posClick.distanceToPoint(atomo.posicion);
        float radio = Atomos::getRadio(atomo.simbolo); // SI no existe devuelve 15.0 A.
        if(distancia < radio){
            return atomo.id;
        }
    }
    return -1;
}

float View::getDistanciaPuntoSegmento(const QVector3D& posClick, const QVector3D& pos1, const QVector3D& pos2){
    QVector3D ab = pos2 - pos1;
    QVector3D ap = posClick - pos1;

    // Calcular la proyección del punto sobre el vector del segmento
    float longitudAbCuadrada = ab.lengthSquared();
    if (longitudAbCuadrada < 0.0001f) return pos1.distanceToPoint(posClick); // Evitar división por cero

    // Factor de proyección (t) acotado entre 0.0 y 1.0 para mantenerse dentro del segmento
    float t = QVector3D::dotProduct(ap, ab) / longitudAbCuadrada;
    t = qMax(0.0f, qMin(1.0f, t));

    // El punto más cercano sobre el segmento de recta
    QVector3D puntoCercano = pos1 + t * ab;

    // Devolvemos la distancia real entre el clic y ese punto cercano
    return posClick.distanceToPoint(puntoCercano);
}

int View::getEnlaces(const Atomo& atomo){
    int totalEnlaces = 0;

    for(const Enlace& enlace: m_listaEnlaces){
        if(enlace.id_atomo1 == atomo.id || enlace.id_atomo2 == atomo.id){
            totalEnlaces += enlace.orden;
        }
    }
    return totalEnlaces;
}

bool View::isEnlacePosible(const Atomo& atomo1, const Atomo& atomo2){
    // 1. Evitar que un átomo se enlace consigo mismo
    if (atomo1.id == atomo2.id) return false;

    // 2. Calcular cuántos enlaces TOTALES acumulados tiene cada átomo actualmente
    int enlacesActuales1 = getEnlaces(atomo1);
    int enlacesActuales2 = getEnlaces(atomo2);

    // 3. Obtener sus valencias máximas (ej: C=4, N=3, O=2, H=1)
    int max1 = Atomos::getMaxValencia(atomo1.simbolo);
    int max2 = Atomos::getMaxValencia(atomo2.simbolo);

    // Regla A: Si cualquiera de los dos ya está al límite de su octeto/dueto, se bloquea
    if ((enlacesActuales1 + 1) > max1 || (enlacesActuales2 + 1) > max2) {
        return false;
    }

    // 4.- Buscamos si ya existe un enlace triple entre estos dos átomos específicos
    for (const Enlace& enlace : m_listaEnlaces) {
        if ((enlace.id_atomo1 == atomo1.id && enlace.id_atomo2 == atomo2.id) ||
            (enlace.id_atomo1 == atomo2.id && enlace.id_atomo2 == atomo1.id)) {

            // Si el enlace actual ya es de orden 3 (Triple), es imposible físicamente
            // que absorba un cuarto orden, sin importar que al Carbono le sobren electrones.
            if (enlace.orden >= 3) {
                return false;
            }
            break;
        }
    }
    return true;
}

// =========================================================================
// FUNCIONES Procesar CLICKS()
// =========================================================================

void View::procesarClickSeleccion(const QVector3D& posClick){
    // 1. Buscamos de forma inmediata si hay un átomo real bajo la coordenada exacta
    int idAtomoBajoCursor = getIdAtomoFromPosicion(posClick);

    // ─── ANCLAJE ORIGINAL DE ARRASTRE INTACTO (NO SE TOCA) ───
    if (m_ultimaPosRat.x() < -5000.0f) {
        m_ultimaPosRat = posClick; // Guarda el inicio del arrastre para el lienzo
        return;
    }

    if (m_ultimaPosRat.x() >= -5000.0f) {
        float deltaX = posClick.x() - m_ultimaPosRat.x();
        float deltaY = posClick.y() - m_ultimaPosRat.y();
        QVector3D vectorTraslacionFinal(deltaX, deltaY, 0.0f);

        // =========================================================================
        // ESCENARIO A: EL USUARIO ESTÁ ARRASTRANDO (MOVIMIENTO CONFIRMADO)
        // =========================================================================
        if (vectorTraslacionFinal.length() > 3.0f) {
            if (!m_listaAtomos.isEmpty()) {
                setEstadoHistorial();
                for (Atomo& atomo : m_listaAtomos) {
                    atomo.posicion += vectorTraslacionFinal;
                }

                m_listaAtomosTrabajo = m_listaAtomos;

                m_estado.estaGuardado   = false;
                m_estado.estaModificado = true;
                notificaEstado();
            }
        }
        // ─── RESETEO ORIGINAL DE ARRASTRE INTACTO (NO SE TOCA) ───
        m_ultimaPosRat = QVector3D(-10000.0f, -10000.0f, -10000.0f);
    }
}

void View::procesarClickDibujo(const QVector3D& posClick){

    // si has pulsado en el vacio creamos un atomo
    if(getIdAtomoFromPosicion(posClick) < 0){
        setEstadoHistorial();
        Atomo nuevoAtomo;

        nuevoAtomo.id       = ++m_contadorId;
        nuevoAtomo.simbolo  = m_atomoActivo;
        nuevoAtomo.posicion = posClick;

        m_listaAtomos.append(nuevoAtomo);

        m_listaAtomosTrabajo = m_listaAtomos; // 🎯 CLONAR AQUÍ tras el append

        notificaEstado();
    }
}

void View::procesarClickCrearEnlace(const QVector3D& posClick){
    int idAtomo1 = getIdAtomoFromPosicion(posClick);

    if(idAtomo1 < 0 ) return;

    // =========================================================================
    // FASE 1: SELECCIÓN DEL PRIMER ÁTOMO (ÁTOMO 1)
    // =========================================================================
    if(m_idAtomoSeleccionado < 0){
        Atomo   atomo1;
        bool    existe = false;

        for(const Atomo& atomo: m_listaAtomos){
            if(idAtomo1 == atomo.id){
                atomo1 = atomo;
                existe = true;
                break;
            }
        }

        // Si existe, miramos si ya está saturado en su estado basal libre
        if(existe){
            int enlacesActuales = getEnlaces(atomo1);
            int maxEnlaces      = Atomos::getMaxValencia(atomo1.simbolo);

            if(enlacesActuales >= maxEnlaces){
                return; // No se puede ni empezar a enlazar desde aquí
            }
        }
        m_idAtomoSeleccionado = idAtomo1;
        notificaEstado();
        return;
    }

    // =========================================================================
    // FASE 2: SELECCIÓN DEL SEGUNDO ÁTOMO (ÁTOMO 2) Y CONECTIVIDAD
    // =========================================================================
    int idAtomo2 = idAtomo1;
    idAtomo1 = m_idAtomoSeleccionado;

    // Al interactuar con un segundo átomo, la transacción actual siempre se cierra
    m_idAtomoSeleccionado = -1;

    Atomo atomo1;
    Atomo atomo2;
    bool existe1 = false;
    bool existe2 = false;

    for(const Atomo& atomo: m_listaAtomos){
        if(atomo.id == idAtomo1){atomo1 = atomo; existe1 = true;}
        if(atomo.id == idAtomo2){atomo2 = atomo; existe2 = true;}
        if(existe1 && existe2) break;
    }

    // 1.- Miramos si YA EXISTE un enlace entre ellos para intentar subir el orden
    for(Enlace& enlace: m_listaEnlaces){
        if((enlace.id_atomo1 == idAtomo1 && enlace.id_atomo2 == idAtomo2) ||
            (enlace.id_atomo1 == idAtomo2 && enlace.id_atomo2 == idAtomo1)){

            // Si ya es un enlace triple, es físicamente imposible subir a cuádruple
            if(enlace.orden >= 3) {
                notificaEstado(); // Limpia el anillo óptico de selección al fallar
                return;
            }

            // Validamos si la valencia química restante de ambos admite subir el orden
            if(isEnlacePosible(atomo1, atomo2)){
                setEstadoHistorial();
                enlace.orden++;
            }

            notificaEstado();
            return; // Salimos de la función al resolver la colisión del par existente
        }
    }

    // 2.- Si el enlace NO EXISTE, intentamos crearlo desde cero como un enlace simple (Orden 1)
    if(isEnlacePosible(atomo1, atomo2)){
        setEstadoHistorial();
        Enlace nuevoEnlace;

        nuevoEnlace.id_atomo1 = idAtomo1;
        nuevoEnlace.id_atomo2 = idAtomo2;
        nuevoEnlace.orden     = 1;

        m_listaEnlaces.append(nuevoEnlace);
    }

    // Si isEnlacePosible fue falso, m_idAtomoSeleccionado ya vale -1 por la línea de arriba,
    // garantizando que se deseleccione el átomo inicial inmediatamente.
    notificaEstado();
}

void View::procesarClickRotacion(const QVector3D& posClick){

}

// =========================================================================
// FUNCIONES DESHACER/REHACER
// =========================================================================
void View::setEstadoHistorial(){
    // 1. Creamos la foto con el estado exacto actual en memoria RAM
    FotoActual foto;
    foto.listaAtomos  = m_listaAtomos;
    foto.listaEnlaces = m_listaEnlaces;

    // 2. Guardamos la foto en la pila de Deshacer
    m_historialDeshacer.push(foto);

    // 3. ¡IMPORTANTE! Limpiamos el futuro (Rehacer).
    // Si el usuario dibuja o enlaza algo nuevo, ya no puede ir "hacia adelante"
    m_historialRehacer.clear();
}

void View::atrasHistorial(){
    if(m_historialDeshacer.isEmpty()) return;

    FotoActual ahora;
    ahora.listaAtomos  = m_listaAtomos;
    ahora.listaEnlaces = m_listaEnlaces;
    m_historialRehacer.push(ahora);

    FotoActual antes = m_historialDeshacer.pop();
    m_listaAtomos   = antes.listaAtomos;
    m_listaEnlaces  = antes.listaEnlaces;

    notificaEstado();
}

void View::adelanteHistorial(){
    if(m_historialRehacer.isEmpty()) return;

    FotoActual ahora;
    ahora.listaAtomos  = m_listaAtomos;
    ahora.listaEnlaces = m_listaEnlaces;
    m_historialDeshacer.push(ahora);

    FotoActual despues = m_historialRehacer.pop();
    m_listaAtomos   = despues.listaAtomos;
    m_listaEnlaces  = despues.listaEnlaces;

    notificaEstado();
}


// =========================================================================
// FUNCIONES LANZAR CALCULOS
// =========================================================================
void View::lanzarCalculoMopac(const QString& ruta, TipoCalculo tipoCalculo){
    QProcess* procesoMopac = new QProcess(this);
    QString rutaBase = ruta.section('.', 0, -2);

    connect(procesoMopac, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, procesoMopac, ruta, rutaBase, tipoCalculo](int exitCode, QProcess::ExitStatus exitStatus) {

                QString rutaOut = rutaBase + ".out";
                QString rutaArc = rutaBase + ".arc";
                QString rutaAux = rutaBase + ".aux";

                if (exitStatus == QProcess::NormalExit && exitCode == 0) {

                    // 🌟 AQUÍ ESTÁ EL CAMBIO: Usamos directamente el 'tipoCalculo' de la lambda.
                    // En la vuelta 1 (OPT) valdrá estrictamente MopacOpt.
                    // En la vuelta 2 (FORCE) valdrá estrictamente MopacFreq.
                    this->cargarMopacResultados(rutaOut, tipoCalculo);

                    // 2. 🎯 EVALUACIÓN DE LA SEGUNDA FASE SECUENCIAL (OPT -> FORCE)
                    if (this->property("esperandoForceSecuencial").toBool()) {
                        // Consumimos y reseteamos la propiedad inmediatamente para cerrar el bucle
                        this->setProperty("esperandoForceSecuencial", false);

                        // Extraemos el método químico base del usuario e inyectamos el comando FORCE puro
                        QString metodo = this->m_argumentos.split(' ', Qt::SkipEmptyParts).first();
                        this->m_argumentos = metodo + " FORCE DIPOLE NOINTER";

                        // Limpieza obligatoria de archivos temporales del primer proceso antes de relanzar
                        QFile::remove(ruta);
                        QFile::remove(rutaOut);
                        QFile::remove(rutaArc);
                        QFile::remove(rutaAux);
                        procesoMopac->deleteLater();

                        // Lanzamos el segundo cálculo de inmediato. Como ahora m_estado.estaOptimizado
                        // es true, correrá FORCE sobre las coordenadas perfectas.
                        this->lanzarCalculoQm();
                        return; // Salida crucial para ceder el control al nuevo QProcess cuántico
                    }

                } else {
                    // Contingencia ante errores catastróficos de MOPAC
                    this->setProperty("esperandoForceSecuencial", false);
                    QString rutaCopiaVisMol = rutaBase + ".vismol";
                    QFile::remove(rutaCopiaVisMol);

                    m_estado.hayProceso = false;
                    notificaEstado();
                }

                // 3. LIMPIEZA PROTOCOLARIA FINAL (Solo se alcanza si todo el flujo secuencial terminó)
                QFile::remove(ruta);
                QFile::remove(rutaOut);
                QFile::remove(rutaArc);
                QFile::remove(rutaAux);

                procesoMopac->deleteLater();
            });

    procesoMopac->start("mopac", QStringList() << ruta);
}

void View::lanzarCalculoGaussian(const QString& ruta, TipoCalculo tipoCalculo){

}

// =========================================================================
// FUNCIONES RECOGER CALCULOS
// =========================================================================

void View::cargarMopacResultados(const QString& rutaOut, TipoCalculo tipoCalculo) {
    QString rutaArc = rutaOut.section('.', 0, -2) + ".arc";
    QString rutaCopiaVisMol = rutaOut.section('.', 0, -2) + ".vismol";

    QFile archivoArc(rutaArc);
    ValoresCalculo nuevosValores;
    bool exitoGlobal = false;

    // 🎯 REPARACIÓN DE FLUJO: Si es un cálculo de frecuencias puro (FORCE),
    // omitimos la verificación estricta del .arc geométrico, ya que MOPAC v23
    // no genera geometría optimizada en esta fase y la RAM ya posee las coordenadas ideales.
    if (tipoCalculo == TipoCalculo::MopacFreq) {
        exitoGlobal = true;
        // Intentamos extraer las constantes moleculares del .out (Energía, Dipolo, Simetría)
        QFile archivoOut(rutaOut);
        if (archivoOut.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream streamOut(&archivoOut);
            Datos::leerMopacArcSummary(streamOut, m_listaAtomos, nuevosValores);
            archivoOut.close();
        }
    }
    else {
        // Flujo estándar intacto para OPT y Trayectorias dinámicas
        if (archivoArc.exists() && archivoArc.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream streamArc(&archivoArc);
            exitoGlobal = Datos::leerMopacArcSummary(streamArc, m_listaAtomos, nuevosValores);
            archivoArc.close();
        }
        else {
            QFile archivoOut(rutaOut);
            if (archivoOut.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream streamOut(&archivoOut);
                exitoGlobal = Datos::leerMopacArcSummary(streamOut, m_listaAtomos, nuevosValores);
                archivoOut.close();
            }
        }
    }

    setEstadoHistorial();

    if (exitoGlobal) {
        // ESCENARIO A: Trayectoria dinámica (.aux)
        if (tipoCalculo == TipoCalculo::MopacTrayectoria) {
            QString rutaAux = rutaOut.section('.', 0, -2) + ".aux";
            QFile archivoAux(rutaAux);
            if (archivoAux.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream streamAux(&archivoAux);
                Datos::leerMopacAuxTrayectoria(streamAux, m_pelicula, m_listaAtomos);
                archivoAux.close();
            }
            if (!m_pelicula.isEmpty()) {
                m_pelicula.last().valoresCalculo = nuevosValores;
            }
        }
        // 🎯 ESCENARIO B: Frecuencias IR combinadas (Leemos del .out)
        else if (tipoCalculo == TipoCalculo::MopacFreq) {
            QFile archivoOut(rutaOut);
            if (archivoOut.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream streamOut(&archivoOut);
                // Aquí llamará a tu nuevo parser de datos.cpp para poblar m_modosVibracionales
                Datos::leerMopacOutSoloFrecuencias(streamOut, m_listaAtomos, m_modosVibracionales);
                archivoOut.close();
            }
        }
        // ESCENARIO C: Optimización convencional (.out)
        else if (tipoCalculo == TipoCalculo::MopacOpt) {
            QFile archivoOut(rutaOut);
            if (archivoOut.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream streamOut(&archivoOut);
                Datos::leerMopacOutSoloCalores(streamOut, m_calores);
                archivoOut.close();
            }
        }

        // 3. RECUPERACIÓN DE ENLACES
        QVector<Atomo> listaAtomosClon;
        QString comentarioClon;
        Datos::abrirVisMol(rutaCopiaVisMol, listaAtomosClon, m_listaEnlaces, comentarioClon);

        if (tipoCalculo == TipoCalculo::MopacTrayectoria) {
            for (int i = 0; i < m_pelicula.size(); ++i) m_pelicula[i].listaEnlaces = m_listaEnlaces;
        }

        // 4. CENTRADO GEOMÉTRICO Y SINCRONIZACIÓN DE TRABAJO
        if (tipoCalculo == TipoCalculo::MopacTrayectoria && !m_pelicula.isEmpty()) {
            for (int i = 0; i < m_pelicula.size(); ++i) centrarListaAtomos(m_pelicula[i].listaAtomos);
        }
        centrarListaAtomos(m_listaAtomos);

        // Clonamos la geometría real optimizada en la de trabajo para el lienzo
        m_listaAtomosTrabajo = m_listaAtomos;

        // 5. CONSOLIDACIÓN DE BANDERAS DE ESTADO
        m_estado.hayTrayectoria = (tipoCalculo == TipoCalculo::MopacTrayectoria && !m_pelicula.isEmpty());
        m_estado.hayFrecuencias = (tipoCalculo == TipoCalculo::MopacFreq && !m_modosVibracionales.isEmpty());

        if (m_estado.hayTrayectoria) {
            m_fotogramaActual = m_pelicula.size() - 1;
            m_listaAtomos        = m_pelicula[m_fotogramaActual].listaAtomos;
            m_listaEnlaces       = m_pelicula[m_fotogramaActual].listaEnlaces;
            m_listaAtomosTrabajo = m_listaAtomos;
        }

        // 6. CONTADORES HISTÓRICOS
        m_valoresCalculo        = nuevosValores;
        m_estado.modoEditor     = ModoEditor::ModoRotacion3D;
        m_estado.estaOptimizado = true;
        m_numeroAtomos          = m_listaAtomos.size();
        m_numeroEnlaces         = m_listaEnlaces.size();
        m_estado.estaModificado = false;
    }

    QFile::remove(rutaCopiaVisMol);
    m_estado.hayProceso = false;
    notificaEstado();
}

TipoCalculo View::getTipoCalculo(){
    QString argsUpper = m_argumentos.toUpper();

    // 🎯 Detecta de forma unívoca la Tarea 2 del combo de la interfaz
    if (argsUpper.contains("AUX") && argsUpper.contains("OPT")) {
        return TipoCalculo::MopacTrayectoria;
    }
    if (argsUpper.contains("FORCE") || argsUpper.contains("FREQ")) {
        return TipoCalculo::MopacFreq;
    }

    return TipoCalculo::MopacOpt;
}

void View::centrarListaAtomos(QVector<Atomo>& lista) {
    if (lista.isEmpty()) return;

    // 1. Inicialización matemática estricta usando el primer elemento real de la RAM
    QVector3D minCoord = lista[0].posicion;
    QVector3D maxCoord = lista[0].posicion;

    // 2. Cálculo del Bounding Box real (Caja contenedora exacta de la molécula)
    for (const Atomo& at : lista) {
        minCoord.setX(qMin(minCoord.x(), at.posicion.x()));
        minCoord.setY(qMin(minCoord.y(), at.posicion.y()));
        minCoord.setZ(qMin(minCoord.z(), at.posicion.z()));

        maxCoord.setX(qMax(maxCoord.x(), at.posicion.x()));
        maxCoord.setY(qMax(maxCoord.y(), at.posicion.y()));
        maxCoord.setZ(qMax(maxCoord.z(), at.posicion.z()));
    }

    // 3. Fórmula exacta del Centroide (Epicentro geométrico del sistema)
    QVector3D centroide = (minCoord + maxCoord) / 2.0f;

    // 4. Traslación Canónica Pura (Arrastrar el centroide al origen 0,0,0)
    // No requiere divisiones artificiales si la caja contenedora anterior es real.
    for (Atomo& at : lista) {
        at.posicion.setX(at.posicion.x() - centroide.x());
        at.posicion.setY(at.posicion.y() - centroide.y());

        // Mantener Z intacto para salvaguardar el centro de giro de la cámara de Qt
    }
}

