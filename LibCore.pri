#-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#
#    .--------------------------------------------------.
#    |  This file is part of NTGraphics                 |
#    |  Created 2018 by NT (https://ttnghia.github.io)  |
#    '--------------------------------------------------'
#                            \o/
#                             |
#                            / |
#
#-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

INCLUDEPATH += $$PWD/Externals/Catch2/single_include
INCLUDEPATH += $$PWD/Externals/glm
INCLUDEPATH += $$PWD/Externals/json/single_include/nlohmann
INCLUDEPATH += $$PWD/Externals/spdlog/include
INCLUDEPATH += $$PWD/Externals/tinyobjloader
INCLUDEPATH += $$PWD/Externals/PartioBgeo

win32: INCLUDEPATH += $$PWD/Externals/tbb_win/include
macx: INCLUDEPATH += $$PWD/Externals/tbb_osx/include
unix: INCLUDEPATH += $$PWD/Externals/tbb_linux/include

INCLUDEPATH += $$PWD/Engine

#-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
CONFIG += c++17
CONFIG += force_debug_info

win32 {
    QMAKE_CXXFLAGS += /std:c++latest
    QMAKE_CXXFLAGS += /MP /W3 /Zc:wchar_t /Zi /Gm- /fp:precise /FC /EHsc
    QMAKE_CXXFLAGS += /D "_WINDOWS" /D "WIN32" /D "WIN64" /D "_MBCS" /D "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS" /D "PARTIO_WIN32"

    CONFIG(debug, debug|release) {
        message("~~~ Debug build ~~~")
        QMAKE_CXXFLAGS += /D "_DEBUG"
        QMAKE_CXXFLAGS += /Od /MDd
    } else {
        QMAKE_CXXFLAGS += /Zo /Qpar /Gy /O2 /Ob2 /Oi /Ot /GF
        QMAKE_CXXFLAGS += /D "NDEBUG"
        static {
            message("~~~ Static Release build ~~~")
            CONFIG += static
            DEFINES += STATIC
            QMAKE_CXXFLAGS += /MT
        } else {
            message("~~~ Dynamic Release build ~~~")
            QMAKE_CXXFLAGS += /MD
        }
    }
}

macx|unix {
    CONFIG += c++1z
    QMAKE_CXXFLAGS += -std=c++17
    QMAKE_CXXFLAGS += -w -g -fpermissive

#    QMAKE_MAC_SDK = macosx10.12
#    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12
    QMAKE_CXXFLAGS_WARN_ON += -Wno-reorder

    CONFIG(debug, debug|release) {
        message("~~~ Debug build ~~~")
        QMAKE_CXXFLAGS += -DDEBUG
    } else {
        message("~~~ Release build ~~~")
        QMAKE_CXXFLAGS += -O3
        QMAKE_CXXFLAGS += -DNDEBUG
    }
}
