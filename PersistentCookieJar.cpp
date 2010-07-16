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

/*
 * Persistent cookie storage for our QWebView.
 *
 * We use DomainNameHelper.{c,h} for validating domains before setting cookies. The
 * DomainNameHelper implements a recognizer for the publicsuffix.org list of top-level
 * domains.
 *
 * The current policy for cookies is:
 *
 *  - cookiesForUrl() returns all cookies that are valid for that URL, including cookies
 *    that are valid for all subdomains for the registered domain of that URL. That is,
 *    a request for "subdomain.bbc.co.uk" would yield all cookies for "subdomain.bbc.co.uk",
 *    but also cookies stored with the special domain ".bbc.co.uk", which means "make available
 *    for all subdomains".
 *
 * - setCookiesFromUrl() validates its cookies against the registered domain of the URL.
 *   In case the cookie domain is empty, the hostname of the URL is used as the cookie's
 *   domain. Cookies for the special "match all subdomains" domain can only be set by URLs
 *   that end in the registered name of the URL's hostname. For all other cookie types, the
 *   cookie domain must exactly match the URL's hostname.
 *
 * One important thing to note is that the class only implements the "make available for all subdomains"
 * on the *registered domain* (i.e. the most top-level domain -- one level up from any TLD). That means
 * that setting a cookie for use in "news.images.bbc.co.uk" can not use the cookie domain
 * ".images.bbc.co.uk". Globally accessible cookies must be set to the registered domain (in this case
 * ".bbc.co.uk").
 */

/*
 * TODO: Handle cookie expiry, and check that any interaction with DomainNameHelper for international
 *       domain names is correct.
 */

#include "PersistentCookieJar.h"

PersistentCookieJar::PersistentCookieJar(QObject *parent) : QNetworkCookieJar(parent) {
}

PersistentCookieJar::~PersistentCookieJar() {
}

QString PersistentCookieJar::safeCookieDomain(QNetworkCookie &cookie, const QUrl &url) {
    if (cookie.domain().isEmpty()) {
        cookie.setDomain(url.host());
    }
    return cookie.domain();
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
            // Setup serialization format
            QDataStream qds(device);
            qds.setVersion(QDataStream::Qt_4_6);
            qds.setByteOrder(QDataStream::LittleEndian);

            // Number of cookies to read.
            quint32 ncookies;
            qds >> ncookies;

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
    QList<QNetworkCookie> cookies;

    QString domain = url.host();
    QString registeredDomain = dnh.getRegisteredDomainPart(domain);
    if (registeredDomain.isEmpty()) {
        qWarning("PersistentCookieJar: Empty registered-domain encountered. Not returning any cookies.");
        return QList<QNetworkCookie>();
    }

    // Is it a valid domain?
    if (domain.length() >= registeredDomain.length() && domain.endsWith(registeredDomain)) {
        // Get all cookies for the subdomain and all cookies for all the domains.
        cookies += storage[domain];
        cookies += storage[QString(".%1").arg(registeredDomain)];
    }

    return cookies;
}

// Set new cookies for an URL
bool PersistentCookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url) {
    QString domain = url.host();
    QString registeredDomain = dnh.getRegisteredDomainPart(domain);
    QList<QNetworkCookie> add;

    if (registeredDomain.isEmpty()) {
        qWarning("PersistentCookieJar: Empty registered-domain.");
        return false;
    }

    if (!(domain.length() >= registeredDomain.length() && domain.endsWith(registeredDomain))) {
        qWarning("PersistentCookieJar: Invalid domain.");
        return false;
    }

    // Go through each cookie, validate it.
    foreach (QNetworkCookie cookie, cookieList) {
        QString cookieDomain = safeCookieDomain(cookie, url);

        // Does it end in our registered domain?
        if (! cookieDomain.endsWith(registeredDomain)) {
            qWarning("PersistentCookieJar: Cookie domain does not end in registered domain. Cookie domain is '%s'.", qPrintable(cookieDomain));
            qWarning("PersistentCookieJar: Cookie data is '%s'.", cookie.toRawForm(QNetworkCookie::NameAndValueOnly).constData());
            break;
        }

        // If it starts with a . (dot), only allow it to be set for
        // the registered domain
        if (cookieDomain.startsWith(QString("."))) {
            if (cookieDomain != QString(".%1").arg(registeredDomain)) {
                qWarning("PersistentCookieJar: Dot cookie attempted to be set for non-registered domain.");
                break;
            }
        }
        // Everything else should be good
        add.push_back(cookie);
    }

    // Replace old cookies with the same unique name.
    QMap<QString, QMap<QByteArray, QNetworkCookie> > cookieMap;
    foreach (QNetworkCookie cookie, add) {
        QString cookieDomain = safeCookieDomain(cookie, url);
        QMap<QByteArray, QNetworkCookie> cookies = cookieMap.value(cookieDomain);
        if (cookies.isEmpty()) {
            QList<QNetworkCookie> domainCookies = storage.value(cookieDomain);
            foreach (QNetworkCookie existingCookie, domainCookies) {
                cookies.insert(existingCookie.name(), existingCookie);
            }
        }
        cookies.insert(cookie.name(), cookie);
        cookieMap.insert(cookieDomain, cookies);
    }

    // Store cookies.
    foreach (QString domain, cookieMap.keys()) {
        QMap<QByteArray, QNetworkCookie> cookies = cookieMap.value(domain);
        QList<QNetworkCookie> uniqueCookies = cookies.values();
#if 0
        qWarning("domain = %s, num cookies = %i", qPrintable(domain), uniqueCookies.count());
#endif
        storage.insert(domain, uniqueCookies);
    }

    return true;
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
    clear();
    foreach (QNetworkCookie cookie, cookieList) {
        QString domain = cookie.domain();
        storage[domain].append(cookie);
    }
}

// Clear cookies.
void PersistentCookieJar::clear() {
    storage.clear();
}
