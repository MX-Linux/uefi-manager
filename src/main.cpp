/**********************************************************************
 *  main.cpp
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

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QTranslator>

#include <memory>

#include "cmd.h"
#include "common.h"
#include "log.h"
#include "mainwindow.h"
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef VERSION
    #define VERSION "?.?.?.?"
#endif

bool isUefi();
bool acquireInstanceLock();

int main(int argc, char *argv[])
{
    const QByteArray option = argc == 2 ? QByteArray(argv[1]) : QByteArray();
    const bool informational = option == "-h" || option == "--help" || option == "--help-all"
                               || option == "-v" || option == "--version";
    std::unique_ptr<QCoreApplication> app;
    if (informational) {
        app = std::make_unique<QCoreApplication>(argc, argv);
    } else {
        if (qEnvironmentVariableIsEmpty("DISPLAY") && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")
            && qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
            qWarning("uefi-manager: no display available (DISPLAY and WAYLAND_DISPLAY are both unset); "
                    "a graphical session is required to run this program.");
            return EXIT_FAILURE;
        }

        if (getuid() == 0) {
            qputenv("XDG_RUNTIME_DIR", "/run/user/0");
            qunsetenv("SESSION_MANAGER");
        }
        // Set Qt platform to XCB (X11) if not already set and we're in X11 environment
        if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM") && !qEnvironmentVariableIsEmpty("DISPLAY")
            && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
            qputenv("QT_QPA_PLATFORM", "xcb");
        }

        app = std::make_unique<QApplication>(argc, argv);
        if (getuid() == 0) {
            qputenv("HOME", "/root");
        }
    }

    QApplication::setOrganizationName("MX-Linux");
    QApplication::setApplicationVersion(VERSION);

    QTranslator qtTran;
    if (qtTran.load("qt_" + QLocale::system().name(), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        QApplication::installTranslator(&qtTran);
    }

    QTranslator qtBaseTran;
    if (qtBaseTran.load("qtbase_" + QLocale::system().name(), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        QApplication::installTranslator(&qtBaseTran);
    }

    QTranslator appTran;
    if (appTran.load(QApplication::applicationName() + '_' + QLocale::system().name(),
                     "/usr/share/" + QApplication::applicationName() + "/locale")) {
        QApplication::installTranslator(&appTran);
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("uefi-manager is a tool for managing UEFI boot entries"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({{"f", "frugal"}, QObject::tr("Perform EFI Stub installation for frugal installation.")});
    parser.addOption({{"t", "test"}, QObject::tr("Run in test mode (bypass UEFI detection for GUI testing).")});

    parser.process(*app);

    QApplication::setWindowIcon(QIcon::fromTheme(QApplication::applicationName()));
    QApplication::setApplicationDisplayName(QObject::tr("UEFI Manager"));

    if (!parser.isSet("test") && !isUefi()) {
        QMessageBox::critical(
            nullptr, QObject::tr("UEFI Manager"),
            QObject::tr("This system doesn't seem to support UEFI, or was not booted in UEFI mode. Exiting."));
        return EXIT_FAILURE;
    }

    // Root guard
    QFile loginUidFile {"/proc/self/loginuid"};
    if (loginUidFile.open(QIODevice::ReadOnly)) {
        const QString loginUid = QString(loginUidFile.readAll()).trimmed();
        loginUidFile.close();
        if (loginUid == "0") {
            QMessageBox::critical(
                nullptr, QObject::tr("Error"),
                QObject::tr(
                    "You seem to be logged in as root, please log out and log in as normal user to use this program."));
            exit(EXIT_FAILURE);
        }
    }
    qDebug().noquote() << QApplication::applicationName() << QObject::tr("version:")
                       << QApplication::applicationVersion();

    // One instance at a time: temporary mounts share one base directory, and cleanup on exit sweeps it
    if (!acquireInstanceLock()) {
        QMessageBox::critical(nullptr, QObject::tr("UEFI Manager"),
                              QObject::tr("UEFI Manager is already running."));
        return EXIT_FAILURE;
    }

    Log startLog;
    MainWindow w(parser);
    if (w.frugalInstallDeclined()) {
        return EXIT_SUCCESS;
    }
    w.show();
    return QApplication::exec();
}

bool isUefi()
{
    QDir dir("/sys/firmware/efi/efivars");
    return dir.exists() && !dir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty();
}

// Hold an exclusive lock for the life of the process. flock() is released by the kernel when the
// process exits, so a crashed instance never leaves a stale lock. The file may belong to another
// user, and fs.protected_regular refuses O_CREAT on it in /tmp, so fall back to opening it as is.
bool acquireInstanceLock()
{
    const char *path = "/tmp/uefi-manager.lock";
    int fd = open(path, O_RDONLY | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (fd != -1) {
        fchmod(fd, 0644); // Ignore the umask so other users can open it too; fails harmlessly if not ours
    } else {
        fd = open(path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    }
    if (fd == -1) {
        qWarning() << "Could not open instance lock" << path;
        return true; // Don't block the user over a lock file we can't use
    }
    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
        close(fd);
        return false;
    }
    return true; // Keep fd open until the process exits
}
