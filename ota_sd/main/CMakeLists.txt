

idf_component_register(
    SRCS
        "main.c"
    INCLUDE_DIRS
        "."
        "../components/iotools"
    REQUIRES
        driver nvs_flash esp_pm app_update iotools
)

