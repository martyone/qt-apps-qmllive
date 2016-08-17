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

#include "livenodeengine.h"
#include "liveruntime.h"
#include "qmlhelper.h"
#include "contentpluginfactory.h"
#include "imageadapter.h"
#include "fontadapter.h"

#include "QtQml/qqml.h"
#include "QtQuick/private/qquickpixmapcache_p.h"

// TODO: create proxy configuration settings, controlled by command line and ui

#ifdef QMLLIVE_DEBUG
#define DEBUG qDebug()
#else
#define DEBUG if (0) qDebug()
#endif

/*!
 * \class LiveNodeEngine
 * \brief The LiveNodeEngine class instantiates QML components in cooperation with LiveHubEngine.
 * \inmodule qmllive
 *
 * LiveNodeEngine provides ways to reload qml documents based incoming requests
 * from a hub. A hub can be connected via a local mode ( LocalPublisher) or
 * remote mode (RemotePublisher with a RemoteReceiver).
 *
 * The pripary use case is to allow loading of QML components instantiating
 * QQuickWindow, i.e., inheriting QML Window. A fallbackView can be set in order
 * to support also QML Item based components.
 *
 * In Addition to showing qml-Files the LiveNodeEngine can be extended by plugins to show any other datatype.
 * One need to set the Plugin path to the right destination and the LiveNodeEngine will load all the plugins
 * it finds there.
 *
 * \sa {Custom Runtime}, {ContentPlugin Example}
 */

/*!
 *   \enum LiveNodeEngine::WorkspaceOption
 *
 *   This enum type controls optional workspace related features:
 *
 *   \value NoWorkspaceOption
 *          No optional feature is enabled.
 *   \value LoadDummyData
 *          Enables loading of dummy QML data - QML documents located in the
 *          "dummydata" subdirectory of the workspace directory.
 */

/*!
 * Standard constructor using \a parent as parent
 */
LiveNodeEngine::LiveNodeEngine(QObject *parent)
    : QObject(parent)
    , m_runtime(new LiveRuntime(this))
    , m_xOffset(0)
    , m_yOffset(0)
    , m_rotation(0)
    , m_delayReload(new QTimer(this))
    , m_pluginFactory(new ContentPluginFactory(this))
    , m_activePlugin(0)
{
    m_delayReload->setInterval(250);
    m_delayReload->setSingleShot(true);
    connect(m_delayReload, SIGNAL(timeout()), this, SLOT(reloadDocument()));
}

/*!
 * The QML engine to be used for loading QML components
 */
QQmlEngine *LiveNodeEngine::qmlEngine() const
{
    return m_qmlEngine;
}

/*!
 * Sets \a qmlEngine as the QML engine to be used for loading QML components
 */
void LiveNodeEngine::setQmlEngine(QQmlEngine *qmlEngine)
{
    Q_ASSERT(!this->qmlEngine());
    Q_ASSERT(qmlEngine);

    m_qmlEngine = qmlEngine;

    connect(m_qmlEngine, &QQmlEngine::warnings, this, &LiveNodeEngine::logErrors);
    qmlEngine->setOutputWarningsToStandardError(false);

    m_qmlEngine->rootContext()->setContextProperty("livert", m_runtime);
}

/*!
 * The QQuickView for displaying QML Item based components, i.e., those not
 * creating own windows.
 */
QQuickView *LiveNodeEngine::fallbackView() const
{
    return m_fallbackView;
}

/*!
 * Sets \a fallbackView as the QQuickView for displaying QML Item based
 * components, i.e., those not creating own windows.
 */
void LiveNodeEngine::setFallbackView(QQuickView *fallbackView)
{
    Q_ASSERT(qmlEngine());
    Q_ASSERT(fallbackView);
    Q_ASSERT(fallbackView->engine() == qmlEngine());

    if (fallbackView && fallbackView->engine() != m_qmlEngine) {
        qCritical() << "LiveNodeEngine::fallbackView must use the QmlEngine instance set as LiveNodeEngine::qmlEngine";
    }

    m_fallbackView = fallbackView;
}

/*!
 * Sets the x-offset \a offset of window
 */
void LiveNodeEngine::setXOffset(int offset)
{
    if (m_activeWindow) {
        m_activeWindow->contentItem()->setX(offset);
    }

    m_xOffset = offset;
}

/*!
 * Returns the current x-offset of the window
 */
int LiveNodeEngine::xOffset() const
{
    return m_xOffset;
}

/*!
 * Sets the y-offset \a offset of window
 */
void LiveNodeEngine::setYOffset(int offset)
{
    if (m_activeWindow) {
        m_activeWindow->contentItem()->setY(offset);
    }

    m_yOffset = offset;
}

/*!
 * Returns the current y-offset of the window
 */
int LiveNodeEngine::yOffset() const
{
    return m_yOffset;
}

/*!
 * Sets the rotation \a rotation of window around the center
 */
void LiveNodeEngine::setRotation(int rotation)
{
    if (m_activeWindow) {
        m_activeWindow->contentItem()->setRotation(0);
        const QPointF center(m_activeWindow->width() / 2, m_activeWindow->height() / 2);
        m_activeWindow->contentItem()->setTransformOriginPoint(center);
        m_activeWindow->contentItem()->setRotation(rotation);
    }

    m_rotation = rotation;
}

/*!
 * Return the current rotation angle
 */
int LiveNodeEngine::rotation() const
{
    return m_rotation;
}

/*!
 * Loads the given \a url onto the qml view. Clears any caches.
 */
void LiveNodeEngine::loadDocument(const QUrl& url)
{
    DEBUG << "LiveNodeEngine::loadDocument: " << url;
    m_activeFile = url;

    if (!m_activeFile.isEmpty())
        reloadDocument();
}

/*!
 * Starts a timer to reload the view with a delay.
 *
 * A delay reload is important to avoid constant reloads, while many changes
 * appear.
 */
void LiveNodeEngine::delayReload()
{
    m_delayReload->start();
}

/*!
 * Checks if the QtQuick Controls module exists for the content adapters
 */
void LiveNodeEngine::checkQmlFeatures()
{
    foreach (const QString &importPath, m_qmlEngine->importPathList()) {
        QDir dir(importPath);
        if (dir.exists("QtQuick/Controls") &&
            dir.exists("QtQuick/Layouts") &&
            dir.exists("QtQuick/Dialogs")) {
            m_quickFeatures |= ContentAdapterInterface::QtQuickControls;
        }
    }
}

QUrl LiveNodeEngine::errorScreenUrl() const
{
    return m_quickFeatures.testFlag(ContentAdapterInterface::QtQuickControls)
        ? QUrl("qrc:/livert/error_qt5_controls.qml")
        : QUrl("qrc:/livert/error_qt5.qml");
}

/*!
 * Reloads the active qml document.
 *
 * Emits documentLoaded() when finished.
 */
void LiveNodeEngine::reloadDocument()
{
    Q_ASSERT(qmlEngine());

    while (!m_activeWindowConnections.isEmpty()) {
        disconnect(m_activeWindowConnections.takeLast());
    }

    if (m_fallbackView && m_activeWindow == m_fallbackView) {
        m_fallbackView->setSource(QUrl());
        m_fallbackView->hide();
    }

    m_activeWindow = 0;

    delete m_object;

    QQuickPixmap::purgeCache();
    m_qmlEngine->clearComponentCache();

    checkQmlFeatures();

    emit logClear();

    const QUrl url = queryDocumentViewer(m_activeFile);

    QScopedPointer<QQmlComponent> component(new QQmlComponent(m_qmlEngine, url));
    m_object = component->create();

    if (!component->isReady()) {
        if (component->isLoading()) {
            qCritical() << "Component did not load synchronously."
                        << "URL:" << url.toString()
                        << "(original URL:" << m_activeFile.toString() << ")";
        } else {
            emit logErrors(component->errors());
            delete m_object;
            if (m_fallbackView) {
                m_fallbackView->setResizeMode(QQuickView::SizeRootObjectToView);
                m_fallbackView->setSource(errorScreenUrl());
                m_activeWindow = m_fallbackView;
            }
        }
    } else if (QQuickWindow *window = qobject_cast<QQuickWindow *>(m_object)) {
        // TODO (why) is this needed?
        m_qmlEngine->setIncubationController(window->incubationController());
        m_activeWindow = window;
    } else if (QQuickItem *item = qobject_cast<QQuickItem *>(m_object)) {
        if (m_fallbackView) {
            const bool hasEmptySize = QSize(item->width(), item->height()).isEmpty();
            if ((m_activePlugin && m_activePlugin->isFullScreen()) || hasEmptySize) {
                m_fallbackView->setResizeMode(QQuickView::SizeRootObjectToView);
            } else {
                m_fallbackView->setResizeMode(QQuickView::SizeViewToRootObject);
            }
            component->setParent(m_fallbackView);
            m_fallbackView->setContent(url, component.take(), m_object);
            m_activeWindow = m_fallbackView;
        } else {
            QQmlError error;
            error.setObject(m_object);
            error.setUrl(url);
            error.setLine(0);
            error.setColumn(0);
            error.setDescription(tr("LiveNodeEngine: Cannot display this component: "
                                    "Root object is not a QQuickWindow and no LiveNodeEngine::fallbackView set."));
            emit logErrors(QList<QQmlError>() << error);
        }
    } else {
        QQmlError error;
        error.setObject(m_object);
        error.setUrl(url);
        error.setLine(0);
        error.setColumn(0);
        error.setDescription(tr("LiveNodeEngine: Cannot display this component: "
                                "Root object is not a QQuickWindow nor a QQuickItem."));
        emit logErrors(QList<QQmlError>() << error);
    }

    if (m_activeWindow) {
        m_activeWindowConnections << connect(m_activeWindow, &QWindow::widthChanged,
                                             this, &LiveNodeEngine::onSizeChanged);
        m_activeWindowConnections << connect(m_activeWindow, &QWindow::heightChanged,
                                             this, &LiveNodeEngine::onSizeChanged);
        onSizeChanged();
    }

    emit documentLoaded();
    emit activeWindowChanged(m_activeWindow);

    // Delay showing the window after activeWindowChanged is handled by
    // WindowWidget::setHostedWindow() - it would be destroyed there anyway.
    // (Applies when this is instantiated for the bench.)
    if (m_activeWindow) {
        m_activeWindow->show();
    }
}


/*!
 * Allows to adapt a \a url to display not native qml documents (e.g. images).
 */
QUrl LiveNodeEngine::queryDocumentViewer(const QUrl& url)
{
    initPlugins();

    foreach (ContentAdapterInterface *adapter, m_plugins) {
        if (adapter->canAdapt(url)) {
            adapter->cleanUp();
            adapter->setAvailableFeatures(m_quickFeatures);

            m_activePlugin = adapter;

            return adapter->adapt(url, m_qmlEngine->rootContext());
        }
    }

    m_activePlugin = 0;

    return url;
}

/*!
 * Sets the document \a document to be shown
 */
void LiveNodeEngine::setActiveDocument(const QString &document)
{
    QUrl url;
    if (!document.isEmpty()) {
        url = QUrl::fromLocalFile(m_workspace.absoluteFilePath(document));
    }

    loadDocument(url);
    emit activateDocument(document);
}

/*!
 * Sets the current workspace to \a path. Documents location will be adjusted based on
 * this workspace path. Certain features can be controled by passing \a options.
 *
 * \sa WorkspaceOptions
 */
void LiveNodeEngine::setWorkspace(const QString &path, WorkspaceOptions options)
{
    Q_ASSERT(qmlEngine());

    m_workspace = QDir(path);

    if (options & LoadDummyData) {
        QmlHelper::loadDummyData(m_qmlEngine, m_workspace.absolutePath());
    }
}

/*!
 * Sets the pluginPath to \a path.
 *
 * The pluginPath will be used to load QmlLive plugins
 * \sa {ContentPlugin Example}
 */
void LiveNodeEngine::setPluginPath(const QString &path)
{
    m_pluginFactory->setPluginPath(path);
}

/*!
 * Returns the current pluginPath
 */
QString LiveNodeEngine::pluginPath() const
{
    return m_pluginFactory->pluginPath();
}

/*!
 * Returns the current active document url.
 */
QUrl LiveNodeEngine::activeDocument() const
{
    return m_activeFile;
}

/*!
 * Returns the active content adapter plugin
 */
ContentAdapterInterface *LiveNodeEngine::activePlugin() const
{
    return m_activePlugin;
}

/*!
 * Returns the current active window.
 * \sa activeWindowChanged()
 */
QQuickWindow *LiveNodeEngine::activeWindow() const
{
    return m_activeWindow;
}

/*!
 * Loads all plugins found in the Pluginpath
 */
void LiveNodeEngine::initPlugins()
{
    if (m_plugins.isEmpty()) {
        m_pluginFactory->load();
        m_plugins.append(m_pluginFactory->plugins());
        m_plugins.append(new ImageAdapter(this));
        m_plugins.append(new FontAdapter(this));
    }
}

/*!
 * Handles size changes and updates the view according
 */
void LiveNodeEngine::onSizeChanged()
{
    Q_ASSERT(m_activeWindow != 0);

    if (m_activeWindow->width() != -1 && m_activeWindow->height() != -1) {
        m_runtime->setScreenWidth(m_activeWindow->width());
        m_runtime->setScreenHeight(m_activeWindow->height());
    }

    setRotation(m_rotation);
}

/*!
 * \fn void LiveNodeEngine::activateDocument(const QString& document)
 *
 * The document \a document was activated
 */

/*!
 * \fn void LiveNodeEngine::logClear()
 *
 * Requested to clear the log
 */

/*!
 * \fn void LiveNodeEngine::logIgnoreMessages(bool)
 *
 * Requsted to ignore the Messages when \a on is true
 */

/*!
 * \fn void LiveNodeEngine::documentLoaded()
 *
 * The signal is emitted when the document has finished loading.
 */

/*!
 * \fn void LiveNodeEngine::activeWindowChanged(QQuickWindow *window)
 *
 * The signal is emitted when the activeWindow has changed by changing or
 * reloading the document. \a window is the newly activated window.
 */

/*!
 * \fn void LiveNodeEngine::logErrors(const QList<QQmlError> &errors)
 *
 * Log the Errors \a errors
 */
