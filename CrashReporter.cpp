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
#include "ConfigDialog.h"
#include "Settings.h"

CrashReporter::CrashReporter(QWidget *parent) : QMainWindow(parent) {
    setupUi(this);
    windowTitle = QString::fromLatin1("Mumble for iOS Beta Crash Reporter %1").arg(qApp->applicationVersion());

    // Restore stored geometry
    Settings *s = Settings::get();
    restoreGeometry(s->mainWindowGeometry());
    // Apply any global settings (if needed)
    s->apply();

    // New log finder
    lhLogHandler = new LogHandler(this);

    // Page load progress bar
    qpbProgressBar = new QProgressBar(this);
    qpbProgressBar->setRange(0, 100);
    qsbStatusBar->addPermanentWidget(qpbProgressBar, 0);
    qsbStatusBar->hide();

    // Create Network Access Manager for the crash reporter
    qnamAccessor = new QNetworkAccessManager(this);

    // Load cookies.
    pcjCookies = new PersistentCookieJar();
    QFile f(CrashReporter::cookieDataFilePath());
    pcjCookies->loadPersistentCookiesFromIODevice(&f);
    qnamAccessor->setCookieJar(pcjCookies);

    // Load home page (crash reporter page)
    on_qaGoHome_triggered();
}

CrashReporter::~CrashReporter() {
    // Work around crash in WebCore::PopupMenu::~PopupMenu()
    loadUrl(QLatin1String("about:blank"));

    // Persist cookies to disk before closing
    QFile f(CrashReporter::cookieDataFilePath());
    pcjCookies->persistCookiesToIODevice(&f);

    // Store geometry data
    Settings *s = Settings::get();
    s->setMainWindowGeometry(saveGeometry());
    s->sync();
}

QString CrashReporter::cookieDataFilePath() {
    QString path = QDesktopServices::storageLocation(QDesktopServices::DataLocation);

    QDir d;
    d.mkpath(path);

    return QDir(path).absoluteFilePath("cookies.qds46");
}

void CrashReporter::clearCookies() {
    pcjCookies->clear();
}

void CrashReporter::loadUrl(const QString &url) {
    // Load the crash reporter page.
    QWebPage *qwpPage = new QWebPage(this);
    qwpPage->setNetworkAccessManager(qnamAccessor);
    qwvWebView->setPage(qwpPage);
    qwvWebView->load(QUrl(url));
}

void CrashReporter::loadHomepage() {
    loadUrl(QLatin1String("http://mumble-ios.appspot.com/crashreporter"));
}

void CrashReporter::injectCrashReporterJavaScript() {
    qWarning("CrashReporter: Injecting crashreporter object into window property in current page.");
    QWebPage *page = qwvWebView->page();
    QWebFrame *frame = page->currentFrame();
    lhLogHandler->setNetworkAccessManager(qnamAccessor);
    frame->addToJavaScriptWindowObject(QLatin1String("crashreporter"), lhLogHandler);
    frame->evaluateJavaScript(QLatin1String("CrashReporterLoaded();"));
}

void CrashReporter::on_qwvWebView_loadFinished(bool ok) {
    if (ok) {
        qsbStatusBar->hide();

        // Check if we've loaded our crash reporter page...
        QString url = qwvWebView->url().toString();
        if (url == QLatin1String("https://mumble-ios.appspot.com/crashreporter") || url == QLatin1String("http://mumble-ios.appspot.com/crashreporter")) {
            injectCrashReporterJavaScript();
        }
    }
}

void CrashReporter::on_qwvWebView_loadProgress(int pct) {
    qsbStatusBar->show();
    qpbProgressBar->setValue(pct);
}

void CrashReporter::on_qwvWebView_statusBarMessage(const QString &message) {
    qsbStatusBar->showMessage(message);
}

void CrashReporter::on_qwvWebView_titleChanged(const QString &message) {
    if (message.isEmpty()) {
        this->setWindowTitle(windowTitle);
    } else {
        this->setWindowTitle(QString::fromLatin1("%1 -- %2").arg(windowTitle, message));
    }
}

void CrashReporter::on_qaGoHome_triggered() {
    loadHomepage();
}

void CrashReporter::on_qaConfiguration_triggered() {
    ConfigDialog *cdDialog = new ConfigDialog(this);
    if (cdDialog->exec() == QDialog::Accepted) {
        if (cdDialog->clearCookies())
            clearCookies();
        loadHomepage();
    }
}

void CrashReporter::on_qaQuit_triggered() {
    qApp->quit();
}

void CrashReporter::on_qaAbout_triggered() {
    QMessageBox *qmb = new QMessageBox(this);
    qmb->setWindowTitle(QString::fromLatin1("About %1").arg(QLatin1String("Mumble for iOS Beta Crash Reporter")));
    qmb->setText(QString::fromLatin1("%1 %2").arg(QLatin1String("Mumble for iOS Beta Crash Reporter"), qApp->applicationVersion()));
    qmb->exec();
    delete qmb;
}

void CrashReporter::on_qaAboutQt_triggered() {
    qApp->aboutQt();
}

void CrashReporter::on_qaHelp_triggered() {
    loadUrl(QLatin1String("http://mumble-ios.appspot.com/crashreporter/help"));
}
