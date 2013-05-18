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

#include "basegraphicsscene.h"

#include "cellscene.h"
#include "basepathscene.h"
#include "toolmanager.h"
#include "worldscene.h"

BaseGraphicsScene::BaseGraphicsScene(SceneType type, QObject *parent)
    : QGraphicsScene(parent)
    , mEventView(0)
    , mType(type)
{
}

WorldScene *BaseGraphicsScene::asWorldScene()
{
    return isWorldScene() ? static_cast<WorldScene*>(this) : 0;
}

CellScene *BaseGraphicsScene::asCellScene()
{
    return isCellScene() ? static_cast<CellScene*>(this) : 0;
}

BasePathScene *BaseGraphicsScene::asPathScene()
{
    return isPathScene() ? static_cast<BasePathScene*>(this) : 0;
}

void BaseGraphicsScene::clearScene()
{
    ToolManager::instance()->beginClearScene(this);
    clear();
    ToolManager::instance()->endClearScene(this);
}

