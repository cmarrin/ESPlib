set(ESPlib ${COMPONENT_DIR}/../../../)
set(esplibFiles
    Application.cpp JsonStreamingParser.cpp BrightnessManager.cpp ButtonManager.cpp
    Clock.cpp HTTPParser.cpp LuaManager.cpp System.cpp TimeWeatherServer.cpp 
    WebFileSystem.cpp Shell.cpp ESPWiFiPortal.cpp LuaWFS.cpp HTTPFetchClient.cpp
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
                    REQUIRES arduino-esp32
                    INCLUDE_DIRS "." ${ESPlib} ${Vesper} ${Lua})

target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-missing-field-initializers)
