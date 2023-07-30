idf_component_register(
    SRCS 
        
        
        "extflash.c"
        "filesystem.c"
    INCLUDE_DIRS 
    "."
    REQUIRES main driver json fatfs littlefs esp_event vfs
)