aux_source_directory(. XFDTD_MATERIAL_SRC)
add_library(xfdtd_material OBJECT ${XFDTD_MATERIAL_SRC})
set(${ALL_OBJECT_FILES} ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:xfdtd_material> PARENT_SCOPE)
