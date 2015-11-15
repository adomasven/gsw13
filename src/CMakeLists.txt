include_directories(SYSTEM ${EIGEN_INCLUDE_DIR})

set(MY_LIBS gsw utils gaussSampler circuit)
foreach(lib ${MY_LIBS})
    add_library(${lib} ${lib}.cpp)
endforeach(lib)
set(LIBS ${LIBS} ${MY_LIBS})
target_link_libraries(circuit gsw)

find_package(NTL)
include_directories(${NTL_INCLUDE_DIR})
set(LIBS ${LIBS} ${NTL_LIBRARY})

find_package(GMP)
include_directories(${GMP_INCLUDE_DIR})
set(LIBS ${LIBS} ${GMP_LIBRARIES})

include_directories(SYSTEM ${CYMRIC_INCLUDE_DIR})
set(LIBS ${LIBS} ${CYMRIC_LIBRARY})

# crypto circuit
add_library(cryptoCircuit cryptoCircuit.cpp)
target_link_libraries(cryptoCircuit ${LIBS})

# gsw-fhe
add_executable(gsw-fhe encryption.cpp)
target_link_libraries(gsw-fhe ${LIBS} cryptoCircuit)

# circuit converter
add_executable(circuit-converter circuit_converter.cpp)
target_link_libraries(circuit-converter ${LIBS})
