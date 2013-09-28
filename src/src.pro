#-------------------------------------------------
#
# Project created by QtCreator 2013-09-25T17:01:21
#
#-------------------------------------------------

QT       += core network

QT       -= gui

TARGET = rainbow
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    handler.cpp \
    configuration.cpp \
    log.cpp \
    folder.cpp \
    webfolder.cpp \
    appfolder.cpp \
    server.cpp \
    request.cpp

HEADERS += \
    handler.h \
    configuration.h \
    log.h \
    folder.h \
    webfolder.h \
    appfolder.h \
    server.h \
    request.h
