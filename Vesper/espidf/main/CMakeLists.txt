
set(ESPlib ${COMPONENT_DIR}/../../../)
set(esplibFiles
    Application.cpp JsonStreamingParser.cpp BrightnessManager.cpp ButtonManager.cpp LittleFSShim.cpp 
    Clock.cpp HTTPParser.cpp LuaManager.cpp System.cpp TimeWeatherServer.cpp 
    WebFileSystem.cpp WebShell.cpp IDFWiFiPortal.cpp
)
list(TRANSFORM esplibFiles PREPEND ${ESPlib}/)

set(Vesper ${COMPONENT_DIR}/../../)
set(vesperFiles Vesper.cpp)
list(TRANSFORM vesperFiles PREPEND ${Vesper}/)

set(Lua ${ESPlib}/lua/lua-5.4.8/src/)
set(luaFiles lapi.c lauxlib.c lbaselib.c lcode.c lcorolib.c lctype.c ldblib.c ldebug.c ldo.c
    ldump.c lfunc.c lgc.c linit.c liolib.c llex.c lmathlib.c lmem.c loadlib.c lobject.c lopcodes.c
    loslib.c lparser.c lstate.c lstring.c lstrlib.c ltable.c ltablib.c ltm.c lundump.c
    lutf8lib.c lvm.c lzio.c)
list(TRANSFORM luaFiles PREPEND ${Lua}/)

idf_component_register(SRCS "main.cpp" ${vesperFiles} ${esplibFiles} ${luaFiles}
                    PRIV_REQUIRES esp_driver_gpio esp_wifi spi_flash nvs_flash esp_http_server dns_server esp_timer esp_driver_tsens app_update
                    INCLUDE_DIRS "." ${ESPlib} ${Vesper} ${Lua})
