cmake_minimum_required(VERSION 3.22)
project(Fem3DResultsPostProcessor VERSION 0.1.0 LANGUAGES CXX)

# Module autonome de post-traitement des résultats « tsa-fem3d-results/1 ».
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Type de compilation" FORCE)
endif()

set(FEM3DPOST_QT_DIR "C:/Qt/6.11.2/msvc2022_64" CACHE PATH "Installation de Qt 6 (msvc2022_64)")
list(APPEND CMAKE_PREFIX_PATH "${FEM3DPOST_QT_DIR}")
# QtCore : JSON et fichiers ; QtGui : uniquement pour l'export PDF de l'outil.
find_package(Qt6 REQUIRED COMPONENTS Core Gui)

if(MSVC)
    add_compile_options(/utf-8 /W4 /permissive- /Zc:__cplusplus)
endif()

# --- Bibliothèque : lecture + calculs + exports, sans interface graphique (QtCore seul) ---
add_library(fem3dpost STATIC
    include/fem3dpost/Results.h
    include/fem3dpost/ResultsReader.h
    include/fem3dpost/Calculations.h
    include/fem3dpost/Tables.h
    include/fem3dpost/Csv.h
    include/fem3dpost/Diagrams.h
    include/fem3dpost/Report.h
    src/ResultsReader.cpp
    src/Calculations.cpp
    src/Tables.cpp
    src/Csv.cpp
    src/Diagrams.cpp
    src/Report.cpp
)
target_include_directories(fem3dpost PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(fem3dpost PUBLIC Qt6::Core)

# --- Outil en ligne de commande ---
add_executable(fem3d_report tools/fem3d_report.cpp tools/PdfExport.cpp tools/PdfExport.h)
target_link_libraries(fem3d_report PRIVATE fem3dpost Qt6::Gui)

# --- Tests (mini-cadre maison) ---
set(FEM3DPOST_EXAMPLE_JSON
    "${CMAKE_CURRENT_SOURCE_DIR}/../examples/output/portique_spatial_resultats.json"
    CACHE FILEPATH "Fichier de résultats réel utilisé par les tests")
add_executable(fem3dpost_tests tests/tests.cpp tests/MiniTest.h)
target_link_libraries(fem3dpost_tests PRIVATE fem3dpost)
target_compile_definitions(fem3dpost_tests PRIVATE FEM3DPOST_EXAMPLE_JSON="${FEM3DPOST_EXAMPLE_JSON}")

enable_testing()
set(_fem3dpost_test_env
    "PATH=path_list_prepend:${FEM3DPOST_QT_DIR}/bin"
    "QT_PLUGIN_PATH=set:${FEM3DPOST_QT_DIR}/plugins")
add_test(NAME fem3dpost_tests COMMAND fem3dpost_tests)
set_tests_properties(fem3dpost_tests PROPERTIES ENVIRONMENT_MODIFICATION "${_fem3dpost_test_env}")

# Essai de bout en bout : génération complète (avec PDF) sur le fichier d'exemple.
add_test(NAME fem3d_report_exemple
    COMMAND fem3d_report "${FEM3DPOST_EXAMPLE_JSON}" --out "${CMAKE_CURRENT_BINARY_DIR}/sortie_exemple" --pdf)
set_tests_properties(fem3d_report_exemple PROPERTIES ENVIRONMENT_MODIFICATION "${_fem3dpost_test_env}")
