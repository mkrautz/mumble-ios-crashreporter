QT += core gui network webkit

TARGET = MumbleiOSBetaCrashReporter
TEMPLATE = app

win32 {
	RC_FILE = mumble-ios-crashreporter.rc
}

macx {
	TARGET = "Mumble for iOS Beta Crash Reporter"
	ICON = mumble.icns
    QMAKE_INFO_PLIST = mumble-ios-crashreporter.plist
    QMAKE_PKGINFO_TYPEINFO = MICR
}

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
