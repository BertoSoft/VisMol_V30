cmake_minimum_required(VERSION 3.19)
project(VisMol_V30 LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

# Habilita el compilador automático de recursos de Qt (RCC)
set(CMAKE_AUTORCC ON)

# Compilación aislada y limpia exclusivamente orientada a la Interfaz Gráfica
qt_add_executable(VisMol_V30
    WIN32 MACOSX_BUNDLE
    main.cpp
    vista.h
    vista.ui
    vista.cpp
    iconos.qrc
    atomos.cpp atomos.h
    lienzo.h lienzo.cpp
    view.h view.cpp
    mat.h mat.cpp
    datos.h datos.cpp
    grafica.h grafica.cpp
    # Mantener el archivo aquí es correcto
)

target_include_directories(VisMol_V30 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(VisMol_V30
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS VisMol_V30
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET VisMol_V30
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
