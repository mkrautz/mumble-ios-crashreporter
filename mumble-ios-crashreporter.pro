QT += core gui network webkit
TARGET = mumble-ios-crashreporter
TEMPLATE = app

SOURCES += \
    main.cpp \
    CrashReporter.cpp \
    PersistentCookieJar.cpp

HEADERS += \
    CrashReporter.h \
    PersistentCookieJar.h

FORMS += \
    CrashReporter.ui

RESOURCES +=
