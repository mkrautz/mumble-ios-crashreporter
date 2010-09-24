/* Copyright (C) 2010 Mikkel Krautz <mikkel@krautz.dk>
   Copyright (C) 2005-2010, Thorvald Natvig <thorvald@natvig.com>

   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QtGui/QApplication>
#include "CrashReporter.h"
#ifdef Q_OS_WIN
# include <windows.h>
#endif

static FILE *fConsole = NULL;

static void mumbleMessageOutput(QtMsgType type, const char *msg) {
    char c;
    switch (type) {
        case QtDebugMsg:
            c='D';
            break;
        case QtWarningMsg:
            c='W';
            break;
        case QtFatalMsg:
            c='F';
            break;
        default:
            c='X';
    }

#define LOG(f, msg) fprintf(f, "<%c>%s %s\n", c, \
                qPrintable(QDateTime::currentDateTime().toString(QLatin1String("yyyy-MM-dd hh:mm:ss.zzz"))), msg); \
        fflush(f)

    LOG(fConsole, msg);
#ifdef Q_OS_MAC
    LOG(stderr, msg);
#endif

    if (type == QtFatalMsg) {
#ifdef Q_OS_WIN
        ::MessageBoxA(NULL, msg, "Mumble", MB_OK | MB_ICONERROR);
#endif
        exit(0);
    }
}

static void setupLogging() {
#ifdef Q_OS_WIN
    _wfopen_s(&fConsole, L"log.txt", L"a+");
#else
    QString path = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    QDir d;
    d.mkpath(path);
    QString logfile = QDir(path).absoluteFilePath("log.txt");
    fConsole = fopen(qPrintable(logfile), "a+");
#endif
    if (fConsole)
        qInstallMsgHandler(mumbleMessageOutput);
}

int main(int argc, char *argv[]) {
    QT_REQUIRE_VERSION(argc, argv, "4.6.0");

    QApplication a(argc, argv);
    a.setApplicationName(QLatin1String("MumbleiOSBetaCrashReporter"));
    a.setApplicationVersion(QLatin1String("1.2.1-rc1"));
    a.setOrganizationName(QLatin1String("Mumble"));
    a.setOrganizationDomain(QLatin1String("mumble.info"));

    setupLogging();

    CrashReporter cr;
    cr.show();

    return a.exec();
}
