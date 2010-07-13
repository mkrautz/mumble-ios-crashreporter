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

#include "PersistentCookieJar.h"

PersistentCookieJar::PersistentCookieJar(QObject *parent) : QNetworkCookieJar(parent) {
}

PersistentCookieJar::~PersistentCookieJar() {
}

void PersistentCookieJar::persistCookiesToIODevice(QIODevice *device) {
    QList<QNetworkCookie> cookies = allCookies();

    // Open device in write-only mode.
    if (device->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QDataStream qds(device);
            qds.setVersion(QDataStream::Qt_4_6);
            qds.setByteOrder(QDataStream::LittleEndian);

            // Number of cookies stored.
            qds << static_cast<quint32>(cookies.count());

            // Serialize each cookie.
            foreach (QNetworkCookie cookie, cookies) {
                QByteArray raw = cookie.toRawForm(QNetworkCookie::Full);
                qds << static_cast<quint32>(raw.length());
                qds.writeRawData(raw.constData(), raw.length());
            }

            device->close();
    } else {
        qWarning("PersistentCookieJar: Unable to persist cookies. Could not open file for writing.");
    }
}

void PersistentCookieJar::loadPersistentCookiesFromIODevice(QIODevice *device) {
    QList<QNetworkCookie> cookies;

    // Open the device
    if (device->open(QIODevice::ReadOnly)) {
            qWarning("PersistentCookieJar: Opened file.");

            // Setup serialization format
            QDataStream qds(device);
            qds.setVersion(QDataStream::Qt_4_6);
            qds.setByteOrder(QDataStream::LittleEndian);

            // Number of cookies to read.
            quint32 ncookies;
            qds >> ncookies;

            qWarning("%u", ncookies);

            // Read in all cookies.
            for (quint32 i = 0; i < ncookies; i++) {
                quint32 nbytes;
                qds >> nbytes;

                QByteArray qbaCookieData(nbytes, 0);
                qds.readRawData(qbaCookieData.data(), nbytes);
                cookies += QNetworkCookie::parseCookies(qbaCookieData);
            }

            device->close();
    } else {
        qWarning("PersistentCookieJar: Unable to load cookies. Could not open file for reading.");
    }

    setAllCookies(cookies);
}

// Fetch all cookies for a particular URL
QList<QNetworkCookie> PersistentCookieJar::cookiesForUrl(const QUrl &url) const {
    qWarning("%s STUB!", __FUNCTION__);
    return QList<QNetworkCookie>();
}

// Set new cookies for an URL
bool PersistentCookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url) {
    qWarning("%s STUB!", __FUNCTION__);
    return false;
}

// Get all cookies
QList<QNetworkCookie> PersistentCookieJar::allCookies() const {
    QList<QNetworkCookie> ret;
    foreach (QList<QNetworkCookie> cookieList, storage.values()) {
         ret += cookieList;
    }
    return ret;
}

// Set all cookies
void PersistentCookieJar::setAllCookies(const QList<QNetworkCookie> &cookieList) {
    storage.clear();
    foreach (QNetworkCookie cookie, cookieList) {
        QString domain = cookie.domain();
        storage[domain].append(cookie);
    }
    qWarning("Set all cookies.");
}
