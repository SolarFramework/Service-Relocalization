## remove Qt dependencies
QT       -= core gui
CONFIG -= qt

QMAKE_PROJECT_DEPTH = 0

## global defintions : target lib name, version
TARGET = SolARService_Relocalization
VERSION=1.0.0
PROJECTDEPLOYDIR = $${PWD}/../deploy

## remove Qt dependencies
CONFIG += c++1z
CONFIG += console
CONFIG += verbose

DEFINES += MYVERSION=\"\\\"$${VERSION}\\\"\"
DEFINES += WITHREMOTING

include(findremakenrules.pri)

CONFIG(debug,debug|release) {
    DEFINES += _DEBUG=1
    DEFINES += DEBUG=1
}

CONFIG(release,debug|release) {
    DEFINES += _NDEBUG=1
    DEFINES += NDEBUG=1
}

win32:CONFIG -= static
win32:CONFIG += shared
QMAKE_TARGET.arch = x86_64 #must be defined prior to include
DEPENDENCIESCONFIG = shared install_recurse
PROJECTCONFIG = QTVS

#NOTE : CONFIG as staticlib or sharedlib,  DEPENDENCIESCONFIG as staticlib or sharedlib, QMAKE_TARGET.arch and PROJECTDEPLOYDIR MUST BE DEFINED BEFORE templatelibconfig.pri inclusion
include ($$shell_quote($$shell_path($${QMAKE_REMAKEN_RULES_ROOT}/templateappconfig.pri)))  # Shell_quote & shell_path required for visual on windows

HEADERS += \
    GrpcServerManager.h


SOURCES += \
    GrpcServerManager.cpp\
    SolARService_Relocalization.cpp

unix {
    LIBS += -ldl

    # Avoids adding install steps manually. To be commented to have a better control over them.
    QMAKE_POST_LINK += "make install install_deps"
}

linux {
    LIBS += -ldl
    LIBS += -L/home/linuxbrew/.linuxbrew/lib # temporary fix caused by grpc with -lre2 ... without -L in grpc.pc
}


macx {
    DEFINES += _MACOS_TARGET_
    QMAKE_MAC_SDK= macosx
    QMAKE_CFLAGS += -mmacosx-version-min=10.7 #-x objective-c++
    QMAKE_CXXFLAGS += -mmacosx-version-min=10.7  -std=c++17 -fPIC#-x objective-c++
    QMAKE_LFLAGS += -mmacosx-version-min=10.7 -v -lstdc++
    INCLUDEPATH += ../../libs/cppast/external/cxxopts/include
    LIBS += -lstdc++ -lc -lpthread
    LIBS += -L/usr/local/lib
}

win32 {
    QMAKE_LFLAGS += /MACHINE:X64
    DEFINES += WIN64 UNICODE _UNICODE
    QMAKE_COMPILER_DEFINES += _WIN64

    # Windows Kit (msvc2013 64)
    LIBS += -L$$(WINDOWSSDKDIR)lib/winv6.3/um/x64 -lshell32 -lgdi32 -lComdlg32
    INCLUDEPATH += $$(WINDOWSSDKDIR)lib/winv6.3/um/x64
}

linux {
    run_install.path = $${TARGETDEPLOYDIR}
    run_install.files = $${PWD}/start_relocalization_service.sh
    CONFIG(release,debug|release) {
        run_install.extra = cp $$files($${PWD}/start_relocalization_service_release.sh) $${PWD}/start_relocalization_service.sh
    }
    CONFIG(debug,debug|release) {
        run_install.extra = cp $$files($${PWD}/start_relocalization_service_debug.sh) $${PWD}/start_relocalization_service.sh
    }
    run_install.CONFIG += nostrip
    INSTALLS += run_install

    run_install_cuda.path = $${TARGETDEPLOYDIR}
    run_install_cuda.files = $${PWD}/start_relocalization_service_cuda.sh
    CONFIG(release,debug|release) {
        run_install_cuda.extra = cp $$files($${PWD}/start_relocalization_service_release_cuda.sh) $${PWD}/start_relocalization_service_cuda.sh
    }
    CONFIG(debug,debug|release) {
        run_install_cuda.extra = cp $$files($${PWD}/start_relocalization_service_debug_cuda.sh) $${PWD}/start_relocalization_service_cuda.sh
    }
    run_install_cuda.CONFIG += nostrip
    INSTALLS += run_install_cuda
}

DISTFILES += \
    SolARService_Relocalization_modules.xml \
    SolARService_Relocalization_modules_cuda.xml \
    SolARService_Relocalization_properties.xml \
    SolARService_Relocalization_properties_cuda.xml \
    docker/SolARServiceRelocalization_cuda.dockerfile \
    docker/build_cuda.sh \
    docker/launch.bat \
    docker/launch_cuda.bat \
    docker/launch_cuda.sh \
    docker/start_server_cuda.sh \
    packagedependencies.txt \
    docker/build.sh \
    docker/launch.bat \
    docker/launch.sh \
    docker/SolARServiceRelocalization.dockerfile \
    docker/start_server.sh \
    start_relocalization_service_debug.sh \
    start_relocalization_service_debug_cuda.sh \
    start_relocalization_service_release.sh \
    start_relocalization_service_release_cuda.sh

xml_files.path = $${TARGETDEPLOYDIR}
xml_files.files =  $$files($${PWD}/SolARService_Relocalization_modules.xml) \
                   $$files($${PWD}/SolARService_Relocalization_properties.xml) \
                   $$files($${PWD}/SolARService_Relocalization_modules_cuda.xml) \
                   $$files($${PWD}/SolARService_Relocalization_properties_cuda.xml)

INSTALLS += xml_files

#NOTE : Must be placed at the end of the .pro
include ($$shell_quote($$shell_path($${QMAKE_REMAKEN_RULES_ROOT}/remaken_install_target.pri)))) # Shell_quote & shell_path required for visual on windows

