/*
 * Copyright 2013, Tim Baker <treectrl@users.sf.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PATHWORLDDOCUMENT_H
#define PATHWORLDDOCUMENT_H

#include "document.h"

#include "global.h"

#include <QList>
#include <QPolygonF>
#include <QRectF>
#include <QSet>

class PathView;
class PathWorld;
class WorldChanger;
class WorldLookup;
class WorldNode;
class WorldPath;
class WorldPathLayer;
class WorldScript;

class PathDocument : public Document
{
    Q_OBJECT
public:
    PathDocument(PathWorld *world, const QString &fileName = QString());
    ~PathDocument();

    void setFileName(const QString &fileName);
    const QString &fileName() const;

    bool save(const QString &filePath, QString &error);

    PathWorld *world() const
    { return mWorld; }

    PathView *view() const
    { return (PathView*)Document::view(); }

    WorldChanger *changer() const
    { return mChanger; }

    WorldLookup *lookup() const
    { return mLookup; }

private slots:
    void afterMoveNode(WorldNode *node);

private:
    PathWorld *mWorld;
    QString mFileName;
    WorldLookup *mLookup;
    WorldChanger *mChanger;
};

#endif // PATHWORLDDOCUMENT_H
