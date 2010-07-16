QT += core gui network webkit
TARGET = mumble-ios-crashreporter
TEMPLATE = app

SOURCES += \
    main.cpp \
    CrashReporter.cpp \
    PersistentCookieJar.cpp \
    DomainNameHelper.cpp \
    LogHandler.cpp \
    ConfigDialog.cpp \
    Settings.cpp

HEADERS += \
    CrashReporter.h \
    PersistentCookieJar.h \
    DomainNameHelper.h \
    LogHandler.h \
    ConfigDialog.h \
    Settings.h

FORMS += \
    CrashReporter.ui \
    ConfigDialog.ui

RESOURCES += \
    mumble-ios-crashreporter.qrc
