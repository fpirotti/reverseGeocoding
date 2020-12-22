#-------------------------------------------------
#
# Project created by QtCreator 2016-10-26T15:36:59
#
#-------------------------------------------------

QT       +=  core gui svg xml
#greaterThan(QT_MAJOR_VERSION, 4): QT += widgets  network xml  #axcontainer sql

TARGET = GCoDer
TEMPLATE = app
VERSION = 2.1.2

MAJOR = 2
MINOR = 1
#VERSION_HEADER = ../version.h

##versiontarget.target = $$VERSION_HEADER
##versiontarget.commands = ../version/debug/version.exe $$MAJOR $$MINOR $$VERSION_HEADER
##versiontarget.depends = FORCE

#PRE_TARGETDEPS += $$VERSION_HEADER
#QMAKE_EXTRA_TARGETS += versiontarget

#F:\dev\curl-master\build\lib\Release
SOURCES += main.cpp\
        mainwindow.cpp \
    worker.cpp \
    dialog.cpp \
    curler.cpp

HEADERS  += mainwindow.h \
    worker.h \
    dialog.h \
    curler.h

FORMS    += mainwindow.ui \
    dialog.ui

RESOURCES += \
    resources.qrc




##INCLUDEPATH +=  $$quote(F:/dev/curl-master/include)


win32 {
    CONFIG(debug, debug|release) {
        QMAKE_LIBDIR +=     $$quote(F:/dev/curl-master/build/lib/Debug)
        QMAKE_LIBS +=   -llibcurl-d_imp
    } else {
        QMAKE_LIBDIR +=     $$quote(/usr/lib/x86_64-linux-gnu)
        QMAKE_LIBS +=   -llibcurl-imp
        }
    }

QMAKE_LIBS +=   -lcurl
#Icon made from <a href="http://www.onlinewebfonts.com/icon">Icon Fonts</a>
## is licensed by CC BY 3.0
win32:RC_ICONS += res/iconfence.ico
