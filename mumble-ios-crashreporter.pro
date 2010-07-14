QT += core gui network webkit
TARGET = mumble-ios-crashreporter
TEMPLATE = app

SOURCES += \
    main.cpp \
    CrashReporter.cpp \
    PersistentCookieJar.cpp \
    DomainNameHelper.cpp

HEADERS += \
    CrashReporter.h \
    PersistentCookieJar.h \
    DomainNameHelper.h

FORMS += \
    CrashReporter.ui

RESOURCES += \
    mumble-ios-crashreporter.qrc
