aux_source_directory(. XFDTD_MATERIAL_SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/ade_method XFDTD_MATERIAL_ADE_METHOD_SRC)
add_library(xfdtd_material OBJECT ${XFDTD_MATERIAL_SRC} ${XFDTD_MATERIAL_ADE_METHOD_SRC})
set(${ALL_OBJECT_FILES} ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:xfdtd_material> PARENT_SCOPE)
