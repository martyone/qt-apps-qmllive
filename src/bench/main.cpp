/****************************************************************************
**
** Copyright (C) 2016 Pelagicore AG
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QmlLive tool.
**
** $QT_BEGIN_LICENSE:GPL-QTAS$
** Commercial License Usage
** Licensees holding valid commercial Qt Automotive Suite licenses may use
** this file in accordance with the commercial license agreement provided
** with the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and The Qt Company.  For
** licensing terms and conditions see https://www.qt.io/terms-conditions.
** For further information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
** SPDX-License-Identifier: GPL-3.0
**
****************************************************************************/

#include <QtGui>
#include <QtWidgets>

#include "hostmodel.h"
#include "options.h"
#include "mainwindow.h"
#include "qmllive_version.h"

class Application : public QApplication
{
    Q_OBJECT

public:
    static Application *create(int &argc, char **argv);

protected:
    Application(int &argc, char **argv);
    static bool isMaster();

    QString serverName() const;
    void setDarkStyle();
    void parseArguments(const QStringList &arguments, Options *options);
};

class MasterApplication : public Application
{
    Q_OBJECT

public:
    MasterApplication(int &argc, char **argv);

private:
    void listenForArguments();
    void applyOptions(const Options &options);

private:
    QPointer<MainWindow> m_window;
};

class SlaveApplication : public Application
{
    Q_OBJECT

public:
    SlaveApplication(int &argc, char **argv);

private:
    void warnAboutIgnoredOptions(const Options &options);
    void forwardArguments();
};

Application *Application::create(int &argc, char **argv)
{
    setApplicationName("QmlLiveBench");
    setOrganizationDomain(QLatin1String(QMLLIVE_ORGANIZATION_DOMAIN));
    setOrganizationName(QLatin1String(QMLLIVE_ORGANIZATION_NAME));

    if (isMaster())
        return new MasterApplication(argc, argv);
    else
        return new SlaveApplication(argc, argv);
}

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
    setAttribute(Qt::AA_NativeWindows, true);
    setAttribute(Qt::AA_ImmediateWidgetCreation, true);

    setDarkStyle();
}

bool Application::isMaster()
{
    Q_ASSERT(!applicationName().isEmpty());
    Q_ASSERT(!organizationDomain().isEmpty() || !organizationName().isEmpty());

    static QSharedMemory *lock = 0;
    static bool retv = false;

    if (lock != 0)
        return retv;

    const QString key = QString::fromLatin1("%1.%2-lock")
        .arg(organizationDomain().isEmpty() ? organizationName() : organizationDomain())
        .arg(applicationName());

    lock = new QSharedMemory(key, qApp);

#ifdef Q_OS_UNIX
    // Ensure there is no stale shared memory segment after crash - call QSharedMemory destructor
    { QSharedMemory(key).attach(); }
#endif

    if (lock->attach(QSharedMemory::ReadOnly)) {
        lock->detach();
        return retv = false;
    }

    if (!lock->create(1))
        return retv = false;

    return retv = true;
}

QString Application::serverName() const
{
    Q_ASSERT(!applicationName().isEmpty());
    Q_ASSERT(!organizationDomain().isEmpty() || !organizationName().isEmpty());

    return QString::fromLatin1("%1.%2-app")
        .arg(organizationDomain().isEmpty() ? organizationName() : organizationDomain())
        .arg(applicationName());
}

void Application::setDarkStyle()
{
    QStyle *style = QStyleFactory::create("fusion");
    if (!style) {
        return;
    }
    setStyle(style);

    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#3D3D3D"));
    palette.setColor(QPalette::WindowText, QColor("#FFFFFF"));
    palette.setColor(QPalette::Base, QColor("#303030"));
    palette.setColor(QPalette::AlternateBase, QColor("#4A4A4A"));
    palette.setColor(QPalette::ToolTipBase, QColor("#FFFFFF"));
    palette.setColor(QPalette::ToolTipText, QColor("#3D3D3D"));
    palette.setColor(QPalette::Text, QColor("#F0F0F0"));
    palette.setColor(QPalette::Button, QColor("#353535"));
    palette.setColor(QPalette::ButtonText, QColor("#FFFFFF"));
    palette.setColor(QPalette::BrightText, QColor("#D0021B"));
    palette.setColor(QPalette::Highlight, QColor("#F19300"));
    palette.setColor(QPalette::HighlightedText, QColor("#1C1C1C"));
    setPalette(palette);
}

void Application::parseArguments(const QStringList &arguments, Options *options)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("QmlLive reloading workbench");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("workspace", "workspace folder to watch. If this points to a QML document, than the directory is asssumed to be the workspace and the file the active document.");
    parser.addPositionalArgument("document", "main QML document to load initially.");

    QCommandLineOption pluginPathOption("pluginpath", "path to qmllive plugins", "pluginpath");
    parser.addOption(pluginPathOption);
    QCommandLineOption importPathOption("importpath", "path to qml import path. Can appear multiple times", "importpath");
    parser.addOption(importPathOption);
    QCommandLineOption stayOnTopOption("stayontop", "keep viewer window on top");
    parser.addOption(stayOnTopOption);
    QCommandLineOption addHostOption("addhost", "add or update remote host configuration and exit", "name,address,port");
    parser.addOption(addHostOption);

    parser.process(arguments);

    options->setPluginPath(parser.value(pluginPathOption));
    options->setImportPaths(parser.values(importPathOption));
    options->setStayOnTop(parser.isSet(stayOnTopOption));

    if (parser.isSet(addHostOption)) {
        foreach (const QString &value, parser.values(addHostOption)) {
            const QStringList split = value.split(QLatin1Char(','));
            if (split.count() != 3) {
                qWarning() << "Invalid argument: " << value;
                parser.showHelp(-1);
            }

            Options::HostOptions host;
            host.name = split.at(0);
            host.address = split.at(1);
            bool ok;
            host.port = split.at(2).toInt(&ok);
            if (!ok) {
                qWarning() << "Port must be specified with a number" << value;
                parser.showHelp(-1);
            }

            options->addHostToAdd(host);
        }
    }

    const QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.count() >= 1) {
        QString argument = positionalArguments.value(0);
        QFileInfo fi(argument);
        if (argument.endsWith(".qml")) {
            qDebug() << "First argument ends with \".qml\". Assuming it is a file.";
            if (!fi.exists() || !fi.isFile()) {
                qWarning() << "Document does not exist or is not a file: " << fi.absoluteFilePath();
                parser.showHelp(-1);
            }
            options->setWorkspace(fi.absolutePath());
            options->setActiveDocument(fi.absoluteFilePath());
        } else {
            qDebug() << "First argument does not ending with \".qml\". Assuming it is a workspace.";
            if (!fi.exists() || !fi.isDir()) {
                qWarning() << "Workspace does not exist or is not a directory: " << fi.absoluteFilePath();
                parser.showHelp(-1);
            }
            options->setWorkspace(fi.absoluteFilePath());
        }
    }
    if (positionalArguments.count() == 2) {
        QString argument = positionalArguments.value(1);
        QFileInfo fi(argument);
        if (argument.endsWith(".qml")) {
            qDebug() << "Second argument ends with \".qml\". Assuming it is a file.";
            if (!fi.exists() || !fi.isFile()) {
                qWarning() << "Document does not exist or is not a file: " << fi.absoluteFilePath();
                parser.showHelp(-1);
            }
            options->setActiveDocument(fi.absoluteFilePath());
        } else {
            qWarning() << "If second argument is present it needs to be a QML document: " << fi.absoluteFilePath();
            parser.showHelp(-1);
        }
    }
}

/*
 * class MasterApplication
 */

MasterApplication::MasterApplication(int &argc, char **argv)
    : Application(argc, argv)
    , m_window(new MainWindow)
{
    Options options;
    parseArguments(arguments(), &options);

    applyOptions(options);

    if (options.hasNoninteractiveOptions()) {
        QTimer::singleShot(0, this, &QCoreApplication::quit);
    } else {
        m_window->init();
        m_window->show();
        listenForArguments();
    }
}

void MasterApplication::listenForArguments()
{
    QLocalServer *server = new QLocalServer(this);

    // Remove possibly stale server socket
    QLocalServer::removeServer(serverName());

    if (!server->listen(serverName())) {
        qWarning() << "Failed to listen on local socket: " << server->errorString();
        return;
    }

    auto handleConnection = [this](QLocalSocket *connection) {
        auto QLocalSocket_error = static_cast<void (QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error);
        connect(connection, QLocalSocket_error, this, [connection]() {
            qWarning() << "Error receiving arguments:" << connection->errorString();
            connection->close();
        });

        connect(connection, &QLocalSocket::readyRead, this, [this, connection]() {
            QStringList arguments;

            QDataStream in(connection);
            in.startTransaction();
            in >> arguments;
            if (!in.commitTransaction())
                return;

            Options options;
            parseArguments(arguments, &options);
            applyOptions(options);

            connection->close();
        });

        connect(connection, &QLocalSocket::disconnected, connection, &QObject::deleteLater);
    };

    connect(server, &QLocalServer::newConnection, this, [this, server, handleConnection]() {
        while (QLocalSocket *connection = server->nextPendingConnection())
            handleConnection(connection);

        m_window->activateWindow();
    });
}

void MasterApplication::applyOptions(const Options &options)
{
    if (!options.workspace().isEmpty())
        m_window->setWorkspace(QDir(options.workspace()).absolutePath());

    if (!options.pluginPath().isEmpty()) {
        if (!m_window->isInitialized())
            m_window->setPluginPath(QDir(options.pluginPath()).absolutePath());
        else
            qDebug() << "Ignoring attempt to set plugin path after initialization.";
    }

    if (!options.importPaths().isEmpty()) {
        if (!m_window->isInitialized())
            m_window->setImportPaths(options.importPaths());
        else
            qDebug() << "Ignoring attempt to set import paths after initialization.";
    }

    if (!options.activeDocument().isEmpty()) {
        m_window->activateDocument(options.activeDocument());
    }

    if (options.stayOnTop()) {
        m_window->setStaysOnTop(true);
    }

    if (!options.hostsToAdd().isEmpty()) {
        if (!m_window->isInitialized()) {
            QSettings s;
            foreach (const Options::HostOptions &host, options.hostsToAdd()) {
                HostModel::addOrUpdateHost(&s, host.name, host.address, host.port);
            }
        } else {
            foreach (const Options::HostOptions &hostOptions, options.hostsToAdd()) {
                Host *host = m_window->hostModel()->host(hostOptions.name);
                if (host == 0) {
                    host = new Host;
                    host->setName(hostOptions.name);
                    m_window->hostModel()->addHost(host);
                }
                host->setAddress(hostOptions.address);
                host->setPort(hostOptions.port);
            }
        }
    }
}

/*
 * class SlaveApplication
 */

SlaveApplication::SlaveApplication(int &argc, char **argv)
    : Application(argc, argv)
{
    qInfo() << "Another instance running. Activating...";

    Options options;
    parseArguments(arguments(), &options);
    warnAboutIgnoredOptions(options);
    forwardArguments();
}

void SlaveApplication::warnAboutIgnoredOptions(const Options &options)
{
    if (!options.pluginPath().isEmpty())
        qWarning() << "Ignoring --pluginpath option";

    if (!options.importPaths().isEmpty())
        qWarning() << "Ignoring --importpaths option";
}

void SlaveApplication::forwardArguments()
{
    QLocalSocket *socket = new QLocalSocket(this);

    auto QLocalSocket_error = static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error);
    connect(socket, QLocalSocket_error, this, [socket]() {
        qCritical() << "Error forwarding arguments:" << socket->errorString();
        exit(1);
    });

    connect(socket, &QLocalSocket::connected, this, [socket]() {
        QDataStream out(socket);
        out << arguments();
        socket->disconnectFromServer();
    });

    connect(socket, &QLocalSocket::disconnected, this, &QCoreApplication::quit);

    socket->connectToServer(serverName());
}

int main(int argc, char** argv)
{
    QScopedPointer<Application> app(Application::create(argc, argv));
    return app->exec();
}

#include "main.moc"
