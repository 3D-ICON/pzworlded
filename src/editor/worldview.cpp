/*
 * Copyright 2012, Tim Baker <treectrl@users.sf.net>
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

#include "worldview.h"

#include "world.h"
#include "worlddocument.h"
#include "worldscene.h"
#include "zoomable.h"

#include <QMouseEvent>

WorldView::WorldView(QWidget *parent)
    : BaseGraphicsView(parent)
{
    QVector<qreal> zoomFactors = zoomable()->zoomFactors();
    zoomable()->setZoomFactors(zoomFactors << 6.0 << 8.0);
}

void WorldView::setScene(WorldScene *scene)
{
    BaseGraphicsView::setScene(scene);

    connect(scene->worldDocument(), SIGNAL(worldResized(QSize)),
            SLOT(worldResized()));

    QPolygonF polygon = scene->cellRectToPolygon(scene->world()->bounds());
    mMiniMapItem = new QGraphicsPolygonItem(polygon);
    addMiniMapItem(mMiniMapItem);
}

WorldScene *WorldView::scene() const
{
    return static_cast<WorldScene*>(mScene);
}

void WorldView::worldResized()
{
    QPolygonF polygon = scene()->cellRectToPolygon(scene()->world()->bounds());
    mMiniMapItem->setPolygon(polygon);
}

void WorldView::mouseMoveEvent(QMouseEvent *event)
{
    QPoint tilePos = scene()->pixelToCellCoordsInt(mapToScene(event->pos()));
    emit statusBarCoordinatesChanged(tilePos.x(), tilePos.y());

    BaseGraphicsView::mouseMoveEvent(event);
}
