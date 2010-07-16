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

#include "Settings.h"

Settings *Settings::singleton = NULL;

static QNetworkProxy::ProxyType local_to_qt_proxy(int type) {
    switch (type) {
        case 0:
            return QNetworkProxy::NoProxy;
        case 1:
            return QNetworkProxy::HttpProxy;
        case 2:
            return QNetworkProxy::Socks5Proxy;
    }
    return QNetworkProxy::NoProxy;
}

Settings::Settings() {
    qsSettings = new QSettings(NULL);
}

Settings::~Settings() {
    delete qsSettings;
}

Settings *Settings::get() {
    if (! Settings::singleton)
        Settings::singleton = new Settings();
    return Settings::singleton;
}

// Apply any global settings (e.g. proxy)
void Settings::apply() {
    QNetworkProxy proxy;
    proxy.setType(local_to_qt_proxy(proxyType()));
    if (proxy.type() != QNetworkProxy::NoProxy) {
        proxy.setHostName(proxyHostname());
        proxy.setPort(static_cast<quint16>(proxyPort()));
        if (! proxyUsername().isEmpty()) {
            proxy.setUser(proxyUsername());
            proxy.setPassword(proxyPassword());
        }
    }
    QNetworkProxy::setApplicationProxy(proxy);
}

void Settings::sync() {
    qsSettings->sync();
}

void Settings::setMainWindowGeometry(const QByteArray &geom) {
    qsSettings->setValue(QLatin1String("UserInterface/MainWindowGeometry"), geom);
}

QByteArray Settings::mainWindowGeometry(const QByteArray &defaultVal) {
    return qsSettings->value(QLatin1String("UserInterface/MainWindowGeometry"), defaultVal).toByteArray();
}

// Set proxy type
void Settings::setProxyType(const int type) {
    qsSettings->setValue(QLatin1String("Network/Proxy/Type"), type);
}

// Get proxy type
int Settings::proxyType() {
    return qsSettings->value(QLatin1String("Network/Proxy/Type")).toInt();
}

// Set proxy hostname
void Settings::setProxyHostname(const QString &hostname) {
    qsSettings->setValue(QLatin1String("Network/Proxy/Hostname"), hostname);
}

// Get proxy hostname
QString Settings::proxyHostname() {
    return qsSettings->value(QLatin1String("Network/Proxy/Hostname")).toString();
}

// Set proxy port
void Settings::setProxyPort(const unsigned int port) {
    qsSettings->setValue(QLatin1String("Network/Proxy/Port"), port);
}

// Get proxy port
unsigned int Settings::proxyPort() {
    return qsSettings->value(QLatin1String("Network/Proxy/Port")).toUInt();
}

// Set proxy username
void Settings::setProxyUsername(const QString &username) {
    qsSettings->setValue(QLatin1String("Network/Proxy/Username"), username);
}

// Get proxy username
QString Settings::proxyUsername() {
    return qsSettings->value(QLatin1String("Network/Proxy/Username")).toString();
}

// Set proxy password
void Settings::setProxyPassword(const QString &password) {
    qsSettings->setValue(QLatin1String("Network/Proxy/Password"), password);
}

// Get proxy password
QString Settings::proxyPassword() {
    return qsSettings->value(QLatin1String("Network/Proxy/Password")).toString();
}
