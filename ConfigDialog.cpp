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
#include "LogHandler.h"

ConfigDialog::ConfigDialog(QWidget *p) : QDialog(p) {
    setupUi(this);

    bClearCookies = false;

    // Load settings
    Settings *s = Settings::get();

    qcbType->setCurrentIndex(s->proxyType());
    if (s->proxyType() == 0)
        on_qcbType_currentIndexChanged(0);
    qleHostname->setText(s->proxyHostname());
    unsigned int port = s->proxyPort();
    if (port == 0)
        qlePort->setText(QString());
    else
        qlePort->setText(QString::number(port));
    qleUsername->setText(s->proxyUsername());
    qlePassword->setText(s->proxyPassword());
}

ConfigDialog::~ConfigDialog() {
}

bool ConfigDialog::clearCookies() {
    return bClearCookies;
}

void ConfigDialog::accept() {
    apply();
    QDialog::accept();
}

// Persist and apply settings.
void ConfigDialog::apply() {
    // Proxy settings.
    Settings *s = Settings::get();
    s->setProxyType(qcbType->currentIndex());
    s->setProxyHostname(qleHostname->text());
    s->setProxyPort(qlePort->text().toUInt());
    s->setProxyUsername(qleUsername->text());
    s->setProxyPassword(qlePassword->text());
    s->apply();
}

void ConfigDialog::on_qdbbButtonBox_clicked(QAbstractButton *b) {
    switch (qdbbButtonBox->standardButton(b)) {
        case QDialogButtonBox::Apply: {
            apply();
            break;
        }
    }
}

void ConfigDialog::on_qcbType_currentIndexChanged(int idx) {
    bool enabled = (idx != 0);
    qleHostname->setEnabled(enabled);
    qlePort->setEnabled(enabled);
    qleUsername->setEnabled(enabled);
    qlePassword->setEnabled(enabled);
}

void ConfigDialog::on_qpbClearCookies_clicked() {
    QMessageBox *qmb = new QMessageBox(this);
    qmb->setIcon(QMessageBox::Question);
    qmb->setWindowTitle(QLatin1String("Clear cookies?"));
    qmb->setText(QLatin1String("Are you sure you want to clear all cookies?"));
    qmb->setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
    if (qmb->exec() == QMessageBox::Yes)
        bClearCookies = true;
    delete qmb;
}

void ConfigDialog::on_qpbShowSubmittedLogs_clicked() {
    LogHandler::showSubmittedCrashLogs();
}

void ConfigDialog::on_qpbDeleteSubmittedLogs_clicked() {
    QMessageBox *qmb = new QMessageBox(this);
    qmb->setIcon(QMessageBox::Question);
    qmb->setWindowTitle(QLatin1String("Delete submitted logs?"));
    qmb->setText(QLatin1String("Are you sure you want to delete all your already-submitted crash logs?"));
    qmb->setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
    if (qmb->exec() == QMessageBox::Yes)
        LogHandler::deleteSubmittedCrashLogs();
    delete qmb;
}
