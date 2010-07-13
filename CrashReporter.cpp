/* Copyright (C) 2010 Mikkel Krautz <mikkel@krautz.dk>

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

#include "CrashReporter.h"
#include "ui_CrashReporter.h"

#include "PersistentCookieJar.h"

CrashReporter::CrashReporter(QWidget *parent) : QMainWindow(parent), ui(new Ui::CrashReporter) {
    ui->setupUi(this);

    windowTitle = QString::fromLatin1("Mumble for iOS Crash Reporter");

    qpbProgressBar = new QProgressBar(this);
    qpbProgressBar->setRange(0, 100);

    ui->qsbStatusBar->addPermanentWidget(qpbProgressBar, 0);
    ui->qsbStatusBar->hide();

    // Create Network Access Manager for the crash reporter
    qnamAccessor = new QNetworkAccessManager(this);

    // Load cookies.
    pcjCookies = new PersistentCookieJar();
    QFile f(CrashReporter::cookieDataFilePath());
    pcjCookies->loadPersistentCookiesFromIODevice(&f);
    qnamAccessor->setCookieJar(pcjCookies);

    // Load the crash reporter page.
    QWebPage *qwpPage = new QWebPage(this);
    qwpPage->setNetworkAccessManager(qnamAccessor);

    ui->qwvWebView->setPage(qwpPage);
    ui->qwvWebView->load(QUrl(QLatin1String("http://mumble-ios.appspot.com/crashreporter")));
}

CrashReporter::~CrashReporter() {
    QFile f(CrashReporter::cookieDataFilePath());
    pcjCookies->persistCookiesToIODevice(&f);

    delete ui;
    delete pcjCookies;
}

QString CrashReporter::cookieDataFilePath() {
    QString path = QDesktopServices::storageLocation(QDesktopServices::DataLocation);

    QDir d;
    d.mkdir(path);

    return QDir(path).absoluteFilePath("cookies.qds46");
}

void CrashReporter::on_qwvWebView_loadFinished(bool ok) {
    if (ok) {
        ui->qsbStatusBar->hide();
    }
}

void CrashReporter::on_qwvWebView_loadProgress(int pct) {
    ui->qsbStatusBar->show();
    qpbProgressBar->setValue(pct);
}

void CrashReporter::on_qwvWebView_statusBarMessage(const QString &message) {
    ui->qsbStatusBar->showMessage(message);
}

void CrashReporter::on_qwvWebView_titleChanged(const QString &message) {
    if (message.isEmpty()) {
        this->setWindowTitle(windowTitle);
    } else {
        this->setWindowTitle(QString::fromLatin1("%1 -- %2").arg(windowTitle, message));
    }
}
