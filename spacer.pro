QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Spacer
TEMPLATE = app

CONFIG += c++14

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    foldermapwidget.cpp

HEADERS += \
    mainwindow.h \
    foldermapwidget.h
