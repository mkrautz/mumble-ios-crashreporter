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

#include "LogHandler.h"

#include <QtGui/QtGui>

#include <stdlib.h>

LogHandler::LogHandler(QObject *p) : QObject(p) {
    qsCrashLogDir = LogHandler::crashLogDirectory();
    qsSubmittedCrashLogDir = LogHandler::submittedCrashLogDirectory();
    qnamAccessManager = NULL;
    sState = LogHandler::Ready;
}

LogHandler::~LogHandler() {
}

void LogHandler::setNetworkAccessManager(QNetworkAccessManager *qnam) {
    qnamAccessManager = qnam;
}

QNetworkAccessManager *LogHandler::networkAccessManager() const {
    return qnamAccessManager;
}

void LogHandler::showSubmittedCrashLogs() {
    QString fileName = LogHandler::submittedCrashLogDirectory();
    QFile f(fileName);
    if (! f.exists()) {
        QMessageBox *qmb = new QMessageBox(NULL);
        qmb->setIcon(QMessageBox::Information);
        qmb->setWindowTitle(QLatin1String("Submitted log directory not found"));
        qmb->setText(QLatin1String("The submitted logs directory does not exist. This most likely means "
                                   "that there are no stored logs on your machine."));
        qmb->exec();
        delete qmb;
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
    }
}

void LogHandler::deleteSubmittedCrashLogs() {
    QString submittedDir = LogHandler::submittedCrashLogDirectory();
    if (submittedDir.isEmpty())
        return;
    QStringList directories;
    QDirIterator iter(submittedDir, QDirIterator::Subdirectories);
    while (iter.hasNext()) {
        iter.next();
        QFileInfo fi = iter.fileInfo();
        if (fi.isDir())
            directories << fi.absoluteFilePath();
        else {
            QFile f(fi.absoluteFilePath());
            f.remove();
        }
    }
    foreach (QString dir, directories) {
        QDir d;
        d.rmdir(dir);
    }
}

QString LogHandler::crashLogDirectory() {
    QStringList possiblePaths;
#if defined(Q_OS_WIN)
    QString username;
    wchar_t *envvar = _wgetenv(L"username");
    if (envvar)
        username = QString::fromWCharArray(envvar);
    QString appdata;
    envvar = _wgetenv(L"APPDATA");
    if (envvar) {
        appdata = QString::fromWCharArray(envvar);
        appdata.replace(QChar('\\'), QChar('/'));
    }

    if (username.isEmpty())
        return QString();

    // These first two are the paths provided by Apple in their documentation. In addition,
    // we query %APPDATA%, and check that as well, in case something weird is going on.
    possiblePaths << QString::fromLatin1("C:/Documents and Settings/%1/Application Data/Apple Computer/Logs/CrashReporter/MobileDevice").arg(username);
    possiblePaths << QString::fromLatin1("C:/Users/%1/AppData/Roaming/Apple Computer/Logs/CrashReporter/MobileDevice").arg(username);
    if (! appdata.isEmpty())
        possiblePaths << QString::fromLatin1("%1/Apple Computer/Logs/CrashReporter/MobileDevice").arg(appdata);
#elif defined(Q_OS_MAC)
    char *home = getenv("HOME");
    QString homeDir;
    if (home)
        homeDir = QString::fromLocal8Bit(home);

    possiblePaths << QString::fromLatin1("%1/Library/Logs/CrashReporter/MobileDevice").arg(homeDir);
    possiblePaths << QString::fromLatin1("%1/Library/Logs/DiagnosticReports/MobileDevice").arg(homeDir);
#endif
    foreach (QString path, possiblePaths) {
        if (QFile::exists(path))
            return path;
    }

    return QString();
}

// Get the path of the 'submitted log directory', i.e. the directory where we copy
// the logs that we've submitted to the server already... In case users want to keep
// them.
QString LogHandler::submittedCrashLogDirectory() {
    QString path = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    QDir d(path);
    d.mkpath(QLatin1String("SubmittedLogs"));
    if (d.exists(QLatin1String("SubmittedLogs")))
        return d.absoluteFilePath("SubmittedLogs");
    return QString();
}

// Lists all available crash reporter devices (that is, directories in the iTunes crash report directory)
//
// Callable from JavaScript.
QStringList LogHandler::availableCrashReporterDevices() {
    if (qsCrashLogDir.isEmpty())
        return QStringList();

    QDir d(qsCrashLogDir);

    // Update the list of safe devices.
    QStringList availDevs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    // Don't include .symbolicated devices.
    qslSafeDeviceNames.clear();
    foreach (QString dev, availDevs) {
        if (dev.endsWith(QLatin1String(".symbolicated")))
            continue;
        qslSafeDeviceNames << dev;
    }

    return qslSafeDeviceNames;
}

// Get a list of the available crash reports for a particular device. This lists the files of a
// device directory in hte iTunes crash report directory that match a particular pattern (in this
// case, we're only interested in the iOS crash logs for 'Mumble').
//
// Callable from JavaScript.
QStringList LogHandler::crashFilesForDevice(const QString &deviceName) {
    // Is this a safe path?
    if (! qslSafeDeviceNames.contains(deviceName))
        return QStringList();

    QFileInfoList files = crashLogPathsForApplication(QLatin1String("Mumble"), deviceName);
    QStringList fileNames;
    foreach (QFileInfo info, files) {
        fileNames << info.fileName();
    }

    // Update list of safe files for this device
    qmSafeDeviceFiles.insert(deviceName, fileNames);

    return fileNames;
}

// Reads the contents of a crash file for a particular device. Returns a byte array.
//
// Callable from JavaScript.
QByteArray LogHandler::contentsOfCrashFile(const QString &deviceName, const QString &fileName) const {
    // No crash log dir set? No cookie.
    if (qsCrashLogDir.isEmpty())
		return QByteArray();

	// Are we accessing a safe file?
    if (! qslSafeDeviceNames.contains(deviceName) || ! qmSafeDeviceFiles[deviceName].contains(fileName))
        return QByteArray();

    QDir d(qsCrashLogDir);
    d.cd(deviceName);
    QString targetFile = d.filePath(fileName);

    QFile f(targetFile);
    f.open(QIODevice::ReadOnly);
    QByteArray contents = f.readAll();
    f.close();

    return contents;
}

// Reads the contents of a crash file for a particular device. Returns a properly-encoded string.
//
// Callable from JavaScript.
QString LogHandler::contentsOfCrashFileAsString(const QString &deviceName, const QString &fileName) const {
    QByteArray qbaContents = contentsOfCrashFile(deviceName, fileName);
    if (!qbaContents.isEmpty()) {
        return QString::fromUtf8(qbaContents.constData(), qbaContents.length());
    }

    return QString();
}

// Returns a QFileInfoList for all crash logs for the iOS appplication 'appName' from the device identified
// by 'deviceName'.
QFileInfoList LogHandler::crashLogPathsForApplication(const QString &appName, const QString &deviceName) const {
    if (qsCrashLogDir.isEmpty())
		return QFileInfoList();

    QDir d(qsCrashLogDir);
    d.cd(deviceName);
    QStringList filters;
    filters << QString::fromLatin1("%1*.crash").arg(appName);
    d.setNameFilters(filters);
    return d.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
}

// List all available crash logs. This is a combination of the return value of
// crashFilesForDevice() For all available crash reporter devices (returned by
// availableCrashReporterDevices()).
QList<DeviceLog> LogHandler::allCrashLogs() {
    QList<DeviceLog> logs;
    foreach (QString device, availableCrashReporterDevices()) {
        foreach (QString file, crashFilesForDevice(device)) {
            logs.push_back(QPair<QString, QString>(device, file));
        }
    }
    return logs;
}

// Start submitting all crash logs.
//
// Callable from JavaScript.
void LogHandler::submitAllCrashLogs() {
    // We can only submit if we're in the ready state.
    if (sState != LogHandler::Ready)
        return;

    iCurrentLog = 0;
    qlSubmitList = allCrashLogs();
    if (qlSubmitList.isEmpty()) {
        QMessageBox *qmb = new QMessageBox(NULL);
        qmb->setIcon(QMessageBox::Information);
        qmb->setWindowTitle(QLatin1String("No logs available"));
        qmb->setText(QLatin1String("No crash logs were found on your computer. Nothing to send."));
        qmb->exec();
        delete qmb;
        return;
    }

    qpdProgress = new QProgressDialog(NULL, Qt::Dialog);
    qpdProgress->setWindowTitle(QLatin1String("Submitting crash logs..."));
    qpdProgress->setLabelText(QLatin1String("Preparing crash logs..."));
    qpdProgress->setWindowModality(Qt::ApplicationModal);
    qpdProgress->resize(400, 100);
    qpdProgress->show();

    QObject::connect(qpdProgress, SIGNAL(canceled()), this, SLOT(logSubmitCancelled()));
    qpdProgress->setAutoReset(false);
    qpdProgress->setMaximum(qlSubmitList.count()-1);

    sState = LogHandler::Submitting;
    submitNextLog();

    qelLoop = new QEventLoop(this);
    qelLoop->exec(QEventLoop::DialogExec);

    delete qpdProgress;
    delete qelLoop;
    qpdProgress = NULL;
    qelLoop = NULL;

    // OK, we've submitted our logs to the server. Let's clean up the logs that
    // were successfully sent.
    removeSubmittedLogs();
}


// This is called when the upload of a crash log finished, to initate the next
// upload.
void LogHandler::submitNextLog() {
    DeviceLog log = qlSubmitList.at(iCurrentLog);
    qpdProgress->setLabelText(QString::fromLatin1("Submitting '%1 from device '%2'").arg(log.second, log.first));
    qpdProgress->setValue(iCurrentLog);
    QNetworkReply *reply = qnamAccessManager->post(QNetworkRequest(QUrl(QLatin1String("https://mumble-ios.appspot.com/crashreporter/send"))),
                                                        contentsOfCrashFile(log.first, log.second));
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(uploadFinished()));
}

// This is called whenever an upload is finished. In here, we check if we've
// uploaded all available crash logs, and if that's the case we check whether
// we've successfully uploaded all our crash logs. If we haven't successfully
// uploaded all of our logs, we display a warning telling the user which logs
// were not uploaded and along with a notice that they should try submitting
// them again sometime in the near future.
void LogHandler::uploadFinished() {
    if (sState != LogHandler::Submitting)
        return;

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (reply->error() == QNetworkReply::NoError) {
        qlSubmittedLogs.append(qlSubmitList.at(iCurrentLog));
    }

    ++iCurrentLog;
    if (iCurrentLog < qlSubmitList.count()) {
        submitNextLog();
    } else {
        sState = LogHandler::Done;
        qpdProgress->setLabelText(QLatin1String("Done submitting crash logs"));
        qpdProgress->setCancelButtonText(QLatin1String("OK"));
    }
}

// This is the callback for the 'Cancel' button. The cancel button
// is re-labeled to 'OK' when all crash logs have been successfully
// uploaded, so we
void LogHandler::logSubmitCancelled() {
    QList<DeviceLog> failedSubmits;

    // We were cancelled while we were uploading. Set our state to
    // done, so any future uploadFinished() signals know that they
    // should stop the submit process.
    if (sState == LogHandler::Submitting) {
        sState = LogHandler::Done;
    // We were OK'd away.
    } else {
        // Compile a list of non-successful submits that we can use
        // to show the user later on.
        foreach (DeviceLog log, qlSubmitList) {
            if (! qlSubmittedLogs.contains(log))
                failedSubmits << log;
        }
    }

    qelLoop->quit();

    // Did any of our submits fail? If so, show
    // a dialog explaining the user what went wrong.
    if (failedSubmits.count() > 0) {
        QMessageBox *qmb = new QMessageBox(NULL);
        qmb->setIcon(QMessageBox::Warning);
        qmb->setWindowTitle(QString("Warning"));
        qmb->setText(QString("An error occured while submitting some of your crash logs.\n"
                             "The crash logs that were not sent to the server are still kept on your computer.\n"
                             "Please try again at a later time.\n"
                             "\n"
                             "If the problem persists, please file a bug on the Mumble for iOS Bug Tracker.\n"));
        qmb->exec();
        delete qmb;
    }
}

// Returns the current state of our operation. See
// LogHandler::State (enum).
//
// Callable from JavaScript.
LogHandler::State LogHandler::currentState() const {
    return sState;
}

// Reset us to the Ready state.
//
// Callable from JavaScript.
void LogHandler::resetState() {
    sState = LogHandler::Ready;
}

// Remove all successfully submitted crash logs.
//
// This method does not actually *remove* them from the system,
// but merely copies them into our own directory in %APPDATA%
// or ~/Library/Application Data/ depending on the platform.
void LogHandler::removeSubmittedLogs() const {
    if (qsCrashLogDir.isEmpty()) {
        qWarning("LogHandler: Empty crash log dir. Not removing logs.");
        return;
    }

    if (qsSubmittedCrashLogDir.isEmpty()) {
        qWarning("LogHandler: Empty submit dir. Not removing logs.");
        return;
    }

    foreach (DeviceLog log, qlSubmittedLogs) {
        QDir d(qsSubmittedCrashLogDir);
        QByteArray contents = contentsOfCrashFile(log.first, log.second);
        if (! d.exists(log.first)) {
            if (! d.mkpath(log.first)) {
                qWarning("LogHandler: Failed to mkpath '%s'. Skipping log.", qPrintable(log.first));
                continue;
            }
        }
        if (! d.cd(log.first)) {
            qWarning("LogHandler: Could not change to device directory. Skipping log.");
            continue;
        }

        QFile f(d.filePath(log.second));
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qint64 written = f.write(contents);
            f.close();
            if (written != -1) {
                if (f.exists()) {
                    QDir crashLogDir(qsCrashLogDir);
                    if (crashLogDir.cd(log.first)) {
                        if (crashLogDir.remove(log.second)) {
                            qWarning("LogHandler: %s successfully removed.", qPrintable(log.second));
                        }
                    } else {
                        qWarning("LogHandler: Could not change to submit subdirectory.");
                    }
                } else {
                    qWarning("LogHandler: Copied file does not exist.");
                }
            } else {
                qWarning("LogHandler: Error while copying file.");
            }
        } else {
            qWarning("LogHandler: Unable to open target file for copying.");
        }
    }
}
