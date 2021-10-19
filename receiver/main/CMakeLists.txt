set(COMPONENT_SRCDIRS ".")
set(COMPONENT_ADD_INCLUDEDIRS "include")
#set(COMPONENT_PRIV_REQUIRES  "arduino klp TMP117")
set(COMPONENT_EMBED_FILES "favicon.ico")
register_component()

spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)