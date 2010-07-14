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
 * C++ port of http://github.com/mkrautz/junkcode/blob/master/publicsuffix/publicsuffix.py
 * Not quite idiomatic C++.
 *
 * TODO: Handle international domain names (in particular, do the correct punycode -> unicode
 *       and unicode -> punycode.
 */

#include "DomainNameHelper.h"
#include <QtGlobal>
#include <algorithm>

PublicSuffixRule::PublicSuffixRule() {
}

PublicSuffixRule::PublicSuffixRule(QString rule) {
    bException = false;
    if (rule.startsWith(QString("!"))) {
        bException = true;
        QStringList split = rule.split(QString("!"));
        rule = split.last();
    }
    qslLabels = rule.split(QString("."));
    std::reverse(qslLabels.begin(), qslLabels.end());
    if (qslLabels.last() == QString("*")) {
        bWildcard = true;
    }
}

PublicSuffixRule::~PublicSuffixRule() {
}

bool PublicSuffixRule::isNull() {
    return qslLabels.isEmpty();
}

QString PublicSuffixRule::key() {
    return qslLabels.first();
}

bool PublicSuffixRule::isException() {
    return bException;
}

bool PublicSuffixRule::isWildcard() {
    return bWildcard;
}

QStringList PublicSuffixRule::labels() {
    return qslLabels;
}

QString PublicSuffixRule::toString() {
    return QString("<PublicSuffixRule %1, exception=%2>").arg(qslLabels.join(QString(".")), bException ? QString("1") : QString("0"));
}

int PublicSuffixRule::numLabels() {
    return qslLabels.count();
}

DomainNameHelper::DomainNameHelper() {
    parsePublicSuffixFile();
}

DomainNameHelper::~DomainNameHelper() {
}

// Parse effective_tld_names.dat
void DomainNameHelper::parsePublicSuffixFile() {
    QFile f(":/effective_tld_names.dat");
    QStringList qslRules;

    // First, read all the rules.
    if (f.open(QIODevice::ReadOnly)) {
        while (! f.atEnd()) {
            QByteArray line = f.readLine();
            if (line.startsWith("//"))
                continue;
            QString str = QString::fromUtf8(line.constData(), line.length()).trimmed();
            if (! str.isEmpty())
                qslRules.append(str);
        }
    }

    // No need to get really fancy. Simply arrange all rules into a QMap
    // where the "last" part of each rule (i.e. "uk" of ".co.uk") is used
    // as the key.
    foreach (QString s, qslRules) {
        PublicSuffixRule psr(s);
        qmRules[psr.key()].append(psr);
    }
}

// Return all labels of a domain in reverse-DNS notation
QStringList DomainNameHelper::domainLabels(const QString &str) const {
    QStringList labels = str.split(QString("."));
    std::reverse(labels.begin(), labels.end());
    return labels;
}

QList<PublicSuffixRule> DomainNameHelper::getMatchingRules(const QString &str) const {
    QStringList labels = domainLabels(str);
    // No rules for this TLD. "If no rules match, the prevailing rule is '*'."
    if (! qmRules.contains(labels.at(0))) {
        return QList<PublicSuffixRule>();
    }
    QList<PublicSuffixRule> rules = qmRules[labels.at(0)];
    QList<PublicSuffixRule> matches;
    foreach (PublicSuffixRule rule, rules) {
        QStringList rule_labels = rule.labels();
        int i;
        for (i = 0; i < rule_labels.length(); i++) {
            QString label = rule_labels.at(i);
            if (label == QString("*"))
                continue;
            else if (label == labels.at(i))
                continue;
            else
                break;
        }
        if (i == rule_labels.length()) {
            matches.push_back(rule);
        }
    }
    return matches;
}

PublicSuffixRule DomainNameHelper::getMatchingRule(const QString &str) const {
    QList<PublicSuffixRule> rules = getMatchingRules(str);
    // If no rules match, the prevailing rule is '*'.
    if (rules.isEmpty()) {
        QStringList labels = domainLabels(str);
        return PublicSuffixRule(QString(labels.at(0)));
    }
    // If more than one rule matches, the prevailing rule is the
    // one which is an exception rule.
    QList<PublicSuffixRule> exceptions;
    foreach (PublicSuffixRule rule, rules) {
        if (rule.isException())
            exceptions.push_back(rule);
    }
    if (! exceptions.isEmpty()) {
        rules = exceptions;
    }
    // If there is no matching exception rule, the prevailing
    // rule is the one with the most labels.
    PublicSuffixRule mostlabels;
    foreach (PublicSuffixRule rule, rules) {
        if (mostlabels.isNull()) {
            mostlabels = rule;
            continue;
        }
        Q_ASSERT(mostlabels.numLabels() != rule.numLabels());
        if (mostlabels.numLabels() < rule.numLabels()) {
            mostlabels = rule;
        }
    }
    return mostlabels;
}

QString DomainNameHelper::getRegisteredDomainPart(const QString &str) const {
    PublicSuffixRule rule = getMatchingRule(str);
    QStringList rule_labels = rule.labels();
    // If the prevailing rule is an exception rule, modify it by
    // removing the leftmost label.
    if (rule.isException()) {
        rule_labels.pop_back();
    }
    QStringList labels = domainLabels(str);
    QStringList parts;
    for (int i = 0; i < labels.length(); i++) {
        if (i == rule_labels.length()) {
            parts.push_back(labels.at(i));
            break;
        }
        if (rule_labels.at(i) == QString("*") || rule_labels.at(i) == labels.at(i)) {
            parts.push_back(labels.at(i));
            continue;
        }
        Q_ASSERT(true == false);
        break;
    }
    if (parts.length() < (rule_labels.length()+1)) {
        return QString();
    }
    std::reverse(parts.begin(), parts.end());
    return parts.join(QString("."));
}
