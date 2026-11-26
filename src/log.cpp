/**********************************************************************
 *
 **********************************************************************
 * Copyright (C) 2024-2026 MX Authors
 *
 * Authors: Adrian <adrian@mxlinux.org>
 *          MX Linux <http://mxlinux.org>
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this package. If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/
#include "log.h"

#include <QDateTime>
#include <QStandardPaths>

Log::Log(const QString &fileName)
{
    // The runtime directory (XDG_RUNTIME_DIR, or a private fallback Qt creates) is mode 0700, so
    // other users can't read the log or block it by leaving their own file at the same path.
    // If there is no runtime directory, the empty name makes open() fail and file logging is skipped.
    QString path = fileName;
    if (path.isEmpty()) {
        const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
        if (!runtimeDir.isEmpty()) {
            path = runtimeDir + "/uefi-manager.log";
        }
    }
    logFile.setFileName(path);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "Could not open log file:" << path;
        return;
    }
    qInstallMessageHandler(Log::messageHandler);
}

void Log::messageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    QTextStream termOut(stdout);
    if (msg.contains('\r')) {
        termOut << msg;
        return;
    }
    termOut << msg << '\n';

    QTextStream out(&logFile);
    out << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz "));
    switch (type) {
    case QtInfoMsg:
        out << QStringLiteral("INF");
        break;
    case QtDebugMsg:
        out << QStringLiteral("DBG");
        break;
    case QtWarningMsg:
        out << QStringLiteral("WRN");
        break;
    case QtCriticalMsg:
        out << QStringLiteral("CRT");
        break;
    case QtFatalMsg:
        out << QStringLiteral("FTL");
        break;
    }
    out << QStringLiteral(": ") << msg << '\n';
}

QString Log::getLog()
{
    return logFile.fileName();
}

void Log::flush()
{
    if (logFile.isOpen()) {
        logFile.flush();
    }
}

QByteArray Log::readLog()
{
    QFile file(getLog());
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

bool Log::hasRelevantContent(qsizetype minimumLineCount)
{
    if (minimumLineCount <= 0) {
        return true;
    }

    QFile file(getLog());
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    qsizetype lineCount = 0;
    while (!file.atEnd()) {
        file.readLine();
        ++lineCount;
        if (lineCount >= minimumLineCount) {
            return true;
        }
    }

    return false;
}
