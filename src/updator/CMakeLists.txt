aux_source_directory(. XFDTD_UPDATOR_SRC)
aux_source_directory("./dispersive_material_update_method" XFDTD_DISPERSIVE_MATERIAL_UPDATOR_SRC)
add_library(xfdtd_updator OBJECT ${XFDTD_UPDATOR_SRC} ${XFDTD_DISPERSIVE_MATERIAL_UPDATOR_SRC})
set(${ALL_OBJECT_FILES} ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:updator> PARENT_SCOPE)
