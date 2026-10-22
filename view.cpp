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

    // 4. Refrescamos todo el sistema
    notificaEstado();
}

void View::eliminarEnlace(int idEnlace) {

    // Guarda la memoria
    setEstadoHistorial();

    // Buscamos el enlace por las identificaciones de sus átomos (o si tuviera un id único)
    // En tu estructura actual, si pasamos el índice directo del vector de enlaces:
    if (idEnlace >= 0 && idEnlace < m_listaEnlaces.size()) {
        m_listaEnlaces.removeAt(idEnlace);

        // ¡NUEVO!: Si se elimina un enlace por el menú contextual, limpiamos la selección por seguridad
        m_idAtomoSeleccionado = -1;

        // Notificamos e invocamos al lienzo para que repinte las líneas inmediatamente
        notificaEstado();
    }
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

        m_estado.modoEditor   = ModoEditor::ModoSeleccion;
        m_estado.estaEmpezado = true;
    } else {
        // Si el archivo falla o está corrupto, nos aseguramos de que el sistema quede cerrado
        m_estado.estaEmpezado = false;
    }

    // 5. Notificar de inmediato al Lienzo y a la Vista
    m_estado.estaModificado = false;
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

void View::optGeometrica(){
    if (m_listaAtomos.isEmpty()) return;

    // 1. Encendemos la bandera de proceso activo en la interfaz
    m_estado.hayProceso = true;
    notificaEstado();

    // 2. Extraemos el nombre base directamente porque m_rutaArchivoActual no es nula
    QString nombreBase = QFileInfo(m_rutaArchivoActual).baseName();

    // Definimos las rutas en el directorio temporal
    QString rutaInputMop   = QDir::tempPath() + "/" + nombreBase + ".mop";
    QString rutaCopiaVisMol = QDir::tempPath() + "/" + nombreBase + ".vismol";

    // ─── PASO A: Crear la copia .vismol temporal para salvaguardar los enlaces ───
    Datos::guardarVisMol(rutaCopiaVisMol, m_listaAtomos, m_listaEnlaces, m_comentario);

    // ─── PASO B: Crear el archivo de entrada .mop para MOPAC ───
    QFile archivoMop(rutaInputMop);
    if (!archivoMop.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_estado.hayProceso = false;
        notificaEstado();
        return;
    }

    QTextStream out(&archivoMop);
    out << "PM7 OPT XYZ NOINTER\n";
    out << "Optimizacion Geometrica Rapida - " << nombreBase << "\n\n";

    // FACTOR DE ESCALA QUÍMICA: 1 Angstrom = 100 unidades/píxeles de escena
    const float PIXELES_POR_ANGSTROM = 100.0f;

    for (const Atomo& a : m_listaAtomos) {
        // Convertimos los píxeles masivos de la pantalla a distancias reales de enlace (Å)
        float xAngstrom = a.posicion.x() / PIXELES_POR_ANGSTROM;
        float yAngstrom = a.posicion.y() / PIXELES_POR_ANGSTROM;

        // 🎯 INYECCIÓN DE RUIDO EN Z: Generamos un pequeño desfase aleatorio entre -0.05 y +0.05 Angstroms
        // para romper la simetría plana artificial y forzar la optimización cuántica 3D real
        float ruidoZ = (((rand() % 100) / 100.0f) - 0.5f) * 0.1f;
        float zAngstrom = (a.posicion.z() / PIXELES_POR_ANGSTROM) + ruidoZ;

        out << a.simbolo << " "
            << xAngstrom << " 1 "
            << yAngstrom << " 1 "
            << zAngstrom << " 1\n";
    }
    archivoMop.close();

    // 3. Lanzar MOPAC en segundo plano de forma asíncrona
    QProcess* procesoMopac = new QProcess(this);

    connect(procesoMopac, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, procesoMopac, rutaInputMop, rutaCopiaVisMol](int exitCode, QProcess::ExitStatus exitStatus) {

                QString rutaOut = rutaInputMop.section('.', 0, -2) + ".out";
                QString rutaArc = rutaInputMop.section('.', 0, -2) + ".arc"; // ◄ Identificamos el .arc

                if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                    this->cargarOutOptNoINter(rutaOut, rutaCopiaVisMol);
                } else {
                    m_estado.hayProceso = false;
                    notificaEstado();
                }

                // 🎯 LIMPIEZA PROTOCOLARIA ABSOLUTA: Borrado de residuos intermedios en /tmp
                QFile::remove(rutaInputMop);   // Elimina .mop
                QFile::remove(rutaCopiaVisMol); // Elimina .vismol backup
                QFile::remove(rutaOut);         // Elimina .out
                QFile::remove(rutaArc);         // ◄ ¡CORREGIDO! Eliminamos el .arc físico

                procesoMopac->deleteLater();
            });

    procesoMopac->start("mopac", QStringList() << rutaInputMop);
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

QString View::getComentario(){
    return m_comentario;   
}

QString View::getRutaActual(){
    return m_rutaArchivoActual;
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

void View::setComentario(const QString& comentario){
    m_comentario = comentario;

    m_estado.estaModificado = true;
    notificaEstado();
}

void View::setRutaActual(const QString& ruta){
    m_rutaArchivoActual = ruta;
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
    m_estado.modoEditor     = ModoEditor::ModoSeleccion;
    m_estado.estaVacio      = true;
    m_estado.estaGuardado   = false;
    m_estado.estaModificado = false;
    m_estado.hayProceso     = false;
    m_estado.estaOptimizado = false;
    m_estado.hayDeshacer    = false;
    m_estado.hayRehacer     = false;

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

void View::cargarOutOptNoINter(const QString& rutaOut, const QString& rutaCopiaVisMol){
    // 1. Abrimos el archivo .out de MOPAC para el parser
    QFile archivoOut(rutaOut);
    if (!archivoOut.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_estado.hayProceso = false;
        notificaEstado();
        return;
    }

    QTextStream streamOut(&archivoOut);

    // Guardamos estado en el historial (Deshacer) antes de mutar la molécula
    setEstadoHistorial();

    // 2. 🎯 ¡EL PARSER!: Delegamos la lectura de coordenadas a la capa de persistencia Datos
    bool exitoParser = Datos::leerOutOptNoInter(streamOut, m_listaAtomos);
    archivoOut.close();

    if (exitoParser) {
        // 3. Restauramos los enlaces puros y metadatos desde la copia de seguridad .vismol
        QVector<Atomo>  listaAtomosClon; // Contenedor temporal requerido por la firma de abrirVisMol
        QString         comentarioClon;

        Datos::abrirVisMol(rutaCopiaVisMol, listaAtomosClon, m_listaEnlaces, comentarioClon);

        m_estado.modoEditor     = ModoEditor::ModoSeleccion;
        m_estado.estaOptimizado = true;
        m_estado.estaModificado = true; // Salta el asterisco '*' de guardado
    }

    // 4. Liberamos la UI y notificamos al Lienzo para que repinte de golpe las posiciones convergidas
    m_estado.hayProceso = false;
    notificaEstado();
}