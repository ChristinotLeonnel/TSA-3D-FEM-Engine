cmake_minimum_required(VERSION 3.21)
project(Fem3D VERSION 0.3.0 LANGUAGES CXX)

# Moteur EF 3D autonome (README.md) : C++20, Eigen, aucune dépendance Qt.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FEM3D_EIGEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../TSA/thirdparty/eigen-3.4.1/include/eigen3" CACHE PATH "Dossier contenant Eigen/")
if(NOT EXISTS "${FEM3D_EIGEN_DIR}/Eigen/Core")
    message(FATAL_ERROR "Eigen introuvable : définir FEM3D_EIGEN_DIR (dossier qui contient Eigen/Core)")
endif()
set(FEM3D_OPENSEES_EXE "${CMAKE_CURRENT_SOURCE_DIR}/../TSA/thirdparty/OpenSees/bin/OpenSees.exe" CACHE FILEPATH "OpenSees pour les benchmarks")

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(fem3d STATIC
    src/Geometry/LocalCoordinateSystem3D.cpp
    src/Materials/ElasticMaterial.cpp
    src/Sections/BeamSection3D.cpp
    src/Elements/BeamElements.cpp
    src/Elements/StaticCondensation.cpp
    src/Model/StructuralModel3D.cpp
    src/Assembly/Assembly.cpp
    src/Solver/LinearSystemSolver.cpp
    src/Results/BeamResponse.cpp
    src/Results/ResultsJsonWriter.cpp
    src/Analysis/LinearStaticAnalysis.cpp
)
target_include_directories(fem3d PUBLIC include)
target_include_directories(fem3d SYSTEM PUBLIC "${FEM3D_EIGEN_DIR}")

# Intégré comme sous-projet (TSA : add_subdirectory), seule la bibliothèque est compilée.
option(FEM3D_BUILD_TESTS "Compiler l'exemple, les tests et les benchmarks OpenSees" ${PROJECT_IS_TOP_LEVEL})
if(NOT FEM3D_BUILD_TESTS)
    return()
endif()

# Exemple d'utilisation : portique spatial, export JSON des résultats
add_executable(fem3d_example examples/fem3d_example.cpp)
target_link_libraries(fem3d_example PRIVATE fem3d)

enable_testing()
add_executable(fem3d_tests
    tests/main.cpp
    tests/unit/test_geometry_and_elements.cpp
    tests/analytical/test_analytical.cpp
    tests/analytical/test_releases.cpp
    tests/analytical/test_springs.cpp
    tests/analytical/test_audit_gaps.cpp
    tests/analytical/test_member_loads.cpp
    tests/integration/test_frames.cpp
)
target_include_directories(fem3d_tests PRIVATE tests)
target_link_libraries(fem3d_tests PRIVATE fem3d)
add_test(NAME fem3d_tests COMMAND fem3d_tests)

# Comparaison avec OpenSees (benchmarks/OpenSees/README.md)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/OpenSees/opensees_benchmarks.cpp")
add_executable(fem3d_opensees_benchmarks benchmarks/OpenSees/opensees_benchmarks.cpp)
target_link_libraries(fem3d_opensees_benchmarks PRIVATE fem3d)
target_compile_definitions(fem3d_opensees_benchmarks PRIVATE
    FEM3D_DEFAULT_OPENSEES="${FEM3D_OPENSEES_EXE}"
    FEM3D_BENCHMARK_DIR="${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/OpenSees")
if(EXISTS "${FEM3D_OPENSEES_EXE}")
    add_test(NAME fem3d_opensees_benchmarks COMMAND fem3d_opensees_benchmarks "${FEM3D_OPENSEES_EXE}")
endif()
endif()
