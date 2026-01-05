
set(ESPlib ${COMPONENT_DIR}/../../../)
set(Vesper ${COMPONENT_DIR}/../../)

idf_component_register(SRCS "main.cpp" ${ESPlib}/Application.cpp
                    PRIV_REQUIRES esp_wifi nvs_flash wifi_provisioning
                    INCLUDE_DIRS "." ${ESPlib} ${Vesper})
