cmake_minimum_required(VERSION 3.16)

# Incluimos todos los archivos .cpp que estén en la ruta RAIZ/src/Editor/ a la compilación
# We include all .cpp files inside ROOT/src/Editor/ to compilation
file(GLOB_RECURSE EDITOR_SOURCES "${ROOT_DIR}/src/Editor/*.cpp" "${ROOT_DIR}/src/Editor/*.c")

# Definimos el ejecutable
# We define the executable
add_executable(Editor ${EDITOR_SOURCES})

# Incluimos y enlazamos SDL3 para utilizar con DearImGui
# We include and link SDL3 to use with DearImGui
target_link_libraries(Editor PRIVATE SDL3-shared Engine lua DearImGui sol2::sol2 SDL3_image::SDL3_image-shared minizip)
target_include_directories(Editor PRIVATE ${SDL3_INCLUDE_DIRS} ${ROOT_DIR}/src/Editor/ ${ROOT_DIR}/src/Engine/ ${LUA_INCLUDE_DIRS} ${IMGUI_INCLUDE_DIRS} ${IMGUI_BACKENDS_DIRS} ${SOL2_INCLUDE_DIRS} ${SDL3_IMAGE_INCLUDE_DIRS} ${MINIZIP_INCLUDE_DIRS})

# Hacemos que el ejecutable tenga un _d cuando se compile en debug
# We make so that the executable has a _d whenever it's compiled on debug
set_target_properties(Editor PROPERTIES
        OUTPUT_NAME "Editor$<$<CONFIG:Debug>:${CMAKE_DEBUG_POSTFIX}>"
)

if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(Editor PROPERTIES WIN32_EXECUTABLE TRUE)
endif()