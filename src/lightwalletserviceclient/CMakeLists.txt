file(GLOB SOURCES *.cpp)
add_executable(lwsd-cli ${SOURCES} ${HEADERS})
target_link_libraries(lwsd-cli lndtools walletcore)

