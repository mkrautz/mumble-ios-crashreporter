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

#ifndef __LOGHANDLER_H__
#define __LOGHANDLER_H__

#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtNetwork/QtNetwork>

typedef QPair<QString, QString> DeviceLog;

class LogHandler : public QObject {
        Q_OBJECT

    // Class data types
    enum State { Ready, Submitting, Done };

    public:
        LogHandler(QObject *p = NULL);
        ~LogHandler();
        void setNetworkAccessManager(QNetworkAccessManager *qnam);
        QNetworkAccessManager *networkAccessManager() const;

    protected:
        State sState;
        QNetworkAccessManager *qnamAccessManager;
        QString qsCrashLogDir;
        QString qsSubmittedCrashLogDir;

        // 'Safe' device names and file names (for a device). Calls to
        // the methods:
        //
        //  - crashFilesForDevice()
        //  - availableCrashReporterDevices()
        //  - contentsOfCrashFile()
        //
        // Update these values when reading their data from disk. They
        // also check against these values when accessing files from disk
        // to pervent malicious JavaScript from reading arbitrary files from
        // the users harddrive.
        QMap<QString, QStringList> qmSafeDeviceFiles;
        QStringList qslSafeDeviceNames;

        QString crashLogDirectory() const;
        QString submittedCrashLogDirectory() const;
        QFileInfoList crashLogPathsForApplication(const QString &deviceName, const QString &appName) const;
        QList<DeviceLog> allCrashLogs();
        void submitNextLog();
        void removeSubmittedLogs() const;

    //
    // State and methods related to the submission process.
    //
    protected:
        QList<DeviceLog> qlSubmitList;
        int iCurrentLog;
        QList<DeviceLog> qlSubmittedLogs;
        QEventLoop *qelLoop;
        QProgressDialog *qpdProgress;
    protected slots:
        void uploadFinished();
        void logSubmitCancelled();

    //
    // JavaScript-exported methods.
    //
    public slots:
        QStringList availableCrashReporterDevices();
        QStringList crashFilesForDevice(const QString &deviceName);
        QByteArray contentsOfCrashFile(const QString &deviceName, const QString &fileName) const;
        QString contentsOfCrashFileAsString(const QString &deviceName, const QString &fileName) const;
        void submitAllCrashLogs();
        State currentState() const;
        void resetState();

};

#endif
