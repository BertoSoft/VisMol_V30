#include "view.h"
#include "atomos.h"

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

void View::nuevoProyecto(){
    limpiarLienzo(); // ya actualiza m_estado
    m_estado.estaEmpezado = true;
    notificaEstado();
}

void View::abrirProyecto(){
    limpiarLienzo(); // ya actualiza m_estado





    m_estado.estaEmpezado = true;
    notificaEstado();
}

void View::cerrarProyecto(){
    limpiarLienzo();
    m_estado.estaEmpezado = false;
    notificaEstado();
}

Estado View::getEstado(){
    return m_estado;
}

QString View::getAtomoActivo(){
    return m_atomoActivo;
}

QVector<Atomo> View::getListaAtomos(){
    return m_listaAtomos;
}

QVector<Enlace> View::getListaEnlaces(){
    return m_listaEnlaces;
}

int View::getIdAtomoSeleccionado(){
    return m_idAtomoSeleccionado;
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
    m_listaEnlaces.clear();

    // 2.- Valores de View
    m_atomoActivo         = "";
    m_idAtomoSeleccionado = -1;
    m_contadorId          = 0;
    m_rutaArchivoActual   = "";
    m_comentario          = "";
    m_argumentos          = "";
    m_ultimaPosRat        = QVector3D(-10000.0f, -10000.0f, -10000.0f);

    // 3.- Reiniciamos las variables de control histórico de disco
    m_numeroAtomos        = 0;
    m_numeroEnlaces       = 0;

    // 4.- Estado base inicial por defecto (Inactivo)
    m_estado.modoEditor   = ModoEditor::ModoSeleccion;
    m_estado.estaEmpezado = false;
    m_estado.estaVacio    = true;
    m_estado.estaGuardado = false;
    m_estado.estaModificado = false;
    m_estado.hayProceso   = false;

    // 5.- Limpieza total del historial físico
    m_historialDeshacer.clear();
    m_historialRehacer.clear();
}

void View::notificaEstado(){
    // 1. Detección automática de vacío
    m_estado.estaVacio = m_listaAtomos.isEmpty() && m_listaEnlaces.isEmpty();

    // 2. Detección automática de modificación (Comparación con el historial guardado)
    if (!m_estado.estaEmpezado) {
        m_estado.estaModificado = false;
        m_estado.estaGuardado   = false;
    }
    else {
        // Un proyecto está modificado si el conteo actual difiere del guardado
        bool haCambiadoContenido = (m_listaAtomos.size() != m_numeroAtomos) ||
                                   (m_listaEnlaces.size() != m_numeroEnlaces);

        m_estado.estaModificado = haCambiadoContenido;

        // Está guardado si tiene una ruta de disco asignada Y además no ha sido modificado
        m_estado.estaGuardado   = (!m_rutaArchivoActual.isEmpty()) && !m_estado.estaModificado;
    }

    // 3. Emitir las señales hacia el Lienzo y la Vista
    emit actualizarLienzo();
    emit actualizarVista(m_estado);
}

void View::limpiarLienzo(){

    resetValores();
}

void View::procesarClickSeleccion(const QVector3D& posClick){
    // 1. Buscamos de forma inmediata si hay un átomo real bajo la coordenada exacta
    int idAtomoBajoCursor = getIdAtomoFromPosicion(posClick);

    // ─── PRIMER CLICK, SOLO SE GUARDA ───
    if (m_ultimaPosRat.x() < -5000.0f) {
        m_ultimaPosRat = posClick; // Guarda el inicio del arrastre para el lienzo
        return;
    }

    // SEGUNDO CLICK, GESTIONAMOS UN SEGUNDO CLICK
    if (m_ultimaPosRat.x() >= -5000.0f) {
        float deltaX = posClick.x() - m_ultimaPosRat.x();
        float deltaY = posClick.y() - m_ultimaPosRat.y();

        // =========================================================================
        // ESCENARIO A: EL USUARIO ESTÁ ARRASTRANDO (MOVIMIENTO CONFIRMADO)
        // =========================================================================
        if (deltaX > 3.0f || deltaY > 3.0f) {
            if (!m_listaAtomos.isEmpty()) {

                setEstadoHistorial();

                for (Atomo& atomo : m_listaAtomos) {
                    atomo.posicion += QVector3D(deltaX, deltaY, 0.0f);
                }

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