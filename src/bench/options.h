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

#pragma once

#include <QtCore>

class Options : public QObject
{
    Q_OBJECT
private:
    explicit Options(QObject *parent = 0);
public:
    static Options *instance();

    QString activeDocument() const;
    void setActiveDocument(const QString &activeDocument);

    QString workspace() const;
    void setWorkspace(const QString &workspace);

    QString pluginPath() const;
    void setPluginPath(const QString &pluginPath);

    QStringList importPaths() const;
    void setImportPaths(const QStringList &importPaths);
    void addImportPath(const QString& path);
    void clearImportPaths();

    bool stayOnTop() const;
    void setStayOnTop(bool stayOnTop);

private:
    static Options *s_instance;
    QString m_activeDocument;
    QString m_workspace;
    QString m_pluginPath;
    QStringList m_importPaths;
    bool m_stayOnTop;
};

