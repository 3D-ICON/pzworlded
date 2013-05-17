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

#include "worldchunkmap.h"

#include "luaworlded.h"
#include "pathdocument.h"
#include "world.h"

#include <QRect>
#include <QRegion>

int WorldChunkSquare::IDMax = -1;
QList<WorldChunkSquare*> WorldChunkSquare::WorldChunkSquareCache;

WorldChunkSquare::WorldChunkSquare(int x, int y, int z) :
    ID(++IDMax),
    x(x),
    y(y),
    z(z),
    tiles(1)
{
}

WorldChunkSquare *WorldChunkSquare::getNew(int x, int y, int z)
{
    if (WorldChunkSquareCache.isEmpty())
        return new WorldChunkSquare(x, y, z);

    WorldChunkSquare *sq = WorldChunkSquareCache.takeFirst();
    sq->ID = ++IDMax;
    sq->x = x;
    sq->y = y;
    sq->z = z;

    return sq;
}

void WorldChunkSquare::discard()
{
    chunk = 0;
//    roomID = -1;
//    room = 0;
    ID = 0;

    tiles.fill(0);

    WorldChunkSquareCache += this;
}

/////

QList<WorldChunk*> WorldChunk::WorldChunkCache;

WorldChunk::WorldChunk(WorldChunkMap *chunkMap) :
    wx(0),
    wy(0),
    mChunkMap(chunkMap)
{
    squares.resize(WorldChunkMap::MaxLevels);
    for (int z = 0; z < squares.size(); z++)
        squares[z] = new SparseSquareGrid(mChunkMap->TilesPerChunk, mChunkMap->TilesPerChunk);
}

#include <QDebug>
#include "progress.h"
void WorldChunk::LoadForLater(int wx, int wy)
{
    this->wx = wx;
    this->wy = wy;

    QRectF bounds = QRectF(/*mChunkMap->getWorldXMinTiles() +*/ this->wx * mChunkMap->TilesPerChunk,
                           /*mChunkMap->getWorldYMinTiles() +*/ this->wy * mChunkMap->TilesPerChunk,
                           mChunkMap->TilesPerChunk, mChunkMap->TilesPerChunk);

//    qDebug() << "WorldChunk::LoadForLater" << bounds;

    // No event processing allowed during these scripts!
    QList<WorldScript*> scripts = mChunkMap->mDocument->lookupScripts(bounds);
//    QList<WorldPath::Path*> paths = mChunkMap->mDocument->lookupPaths(bounds);
    qDebug() << QString::fromLatin1("Running scripts (%1) #paths=%2").arg(scripts.size())/*.arg(paths.size())*/;

    foreach (WorldScript *ws, scripts) {
        QRect b = ws->mRegion.boundingRect();
        Tiled::Lua::LuaScript ls(mChunkMap->mWorld, ws);
        ls.runFunction("run");
    }
}

void WorldChunk::setSquare(int x, int y, int z, WorldChunkSquare *square)
{
    squares[z]->replace(x, y, square);
    if (square)
        square->chunk = this;
}

WorldChunkSquare *WorldChunk::getGridSquare(int x, int y, int z)
{
    if ((z >= WorldChunkMap::MaxLevels) || (z < 0)) {
        return 0;
    }
    return const_cast<WorldChunkSquare*>(squares[z]->at(x, y));
}

void WorldChunk::reuseGridsquares()
{
    for (int z = 0; z < squares.size(); z++) {
        squares[z]->beginIterate();
        while (WorldChunkSquare *sq = squares[z]->nextIterate()) {
            sq->discard();
        }
        squares[z]->clear();
    }
}

void WorldChunk::Save(bool bSaveQuit)
{
    // save out to a .bin file here

    reuseGridsquares();
}

WorldChunk *WorldChunk::getNew(WorldChunkMap *chunkMap)
{
    if (WorldChunkCache.isEmpty())
        return new WorldChunk(chunkMap);

    WorldChunk *chunk = WorldChunkCache.takeFirst();
    chunk->mChunkMap = chunkMap;
    return chunk;
}

void WorldChunk::discard()
{
    wx = -1, wy = -1;
    WorldChunkCache += this;
}

/////

class CMTileSink : public WorldTileLayer::TileSink
{
public:
    CMTileSink(WorldChunkMap *chunkMap, WorldTileLayer *wtl) :
        mChunkMap(chunkMap),
        mLayer(wtl),
        mLayerIndex(chunkMap->mWorld->indexOf(wtl))
    {

    }

    void putTile(int x, int y, WorldTile *tile)
    {
        WorldChunkSquare *sq = mChunkMap->getGridSquare(x, y, mLayer->mLevel);
        if (!sq && !tile) return;
        if (!sq && QRect(mChunkMap->getWorldXMinTiles(),
                         mChunkMap->getWorldYMinTiles(),
                         mChunkMap->getWidthInTiles(),
                         mChunkMap->getWidthInTiles()).contains(x,y)) {
            if (WorldChunk *chunk = mChunkMap->getChunkForWorldPos(x, y)) {
                sq = WorldChunkSquare::getNew(x, y, mLayer->mLevel);
                chunk->setSquare(x - chunk->wx * mChunkMap->TilesPerChunk,
                                 y - chunk->wy * mChunkMap->TilesPerChunk,
                                 mLayer->mLevel, sq);
            }
        }
        if (sq) {
            if (sq->tiles.size() < mLayerIndex + 1)
                sq->tiles.resize(mLayerIndex + 1);
            sq->tiles[mLayerIndex] = tile;
        }

    }

    WorldTile *getTile(int x, int y)
    {
        return 0; // FIXME
    }

    WorldChunkMap *mChunkMap;
    WorldTileLayer *mLayer;
    int mLayerIndex;
};

WorldChunkMap::WorldChunkMap(PathDocument *doc) :
    mDocument(doc),
    mWorld(doc->world()),
    WorldX(5),
    WorldY(5),
    XMinTiles(-1),
    XMaxTiles(-1),
    YMinTiles(-1),
    YMaxTiles(-1)
{
    Chunks.resize(ChunkGridWidth);
    for (int x = 0; x < ChunkGridWidth; x++)
        Chunks[x].resize(ChunkGridWidth);

    foreach (WorldTileLayer *wtl, mWorld->tileLayers()) {
        wtl->mSinks += new CMTileSink(this, wtl);
    }
}

void WorldChunkMap::LoadChunkForLater(int wx, int wy, int x, int y)
{
    WorldChunk *chunk = WorldChunk::getNew(this);
    setChunk(x, y, chunk);

    chunk->LoadForLater(wx, wy);
}

WorldChunkSquare *WorldChunkMap::getGridSquare(int x, int y, int z)
{
    x -= getWorldXMin() * TilesPerChunk;
    y -= getWorldYMin() * TilesPerChunk;

    if ((x < 0) || (y < 0) || (x >= getWidthInTiles()) || (y >= getWidthInTiles()) || (z < 0) || (z > MaxLevels)) {
        return 0;
    }

    WorldChunk *c = getChunk(x / TilesPerChunk, y / TilesPerChunk);
    if (c == 0)
        return 0;
    return c->getGridSquare(x % TilesPerChunk, y % TilesPerChunk, z);
}

WorldChunk *WorldChunkMap::getChunkForWorldPos(int x, int y)
{
    x -= getWorldXMin() * TilesPerChunk;
    y -= getWorldYMin() * TilesPerChunk;

    if ((x < 0) || (y < 0) || (x >= getWidthInTiles()) || (y >= getWidthInTiles()) /*|| (z < 0) || (z > MaxLevels)*/) {
        return 0;
    }

    return getChunk(x / TilesPerChunk, y / TilesPerChunk);
}

WorldChunk *WorldChunkMap::getChunk(int x, int y)
{
    if ((x < 0) || (x >= ChunkGridWidth) || (y < 0) || (y >= ChunkGridWidth)) {
        return 0;
    }
    return Chunks[x][y];
}

void WorldChunkMap::setChunk(int x, int y, WorldChunk *c)
{
    if (x < 0 || x >= ChunkGridWidth || y < 0 || y >= ChunkGridWidth) return;
    if (c && Chunks[x][y]) {
        int i = 0;
    }

    Q_ASSERT(!c || (Chunks[x][y] == 0));
    Chunks[x][y] = c;
}

void WorldChunkMap::UpdateCellCache()
{
#if 0
    int i = getWidthInTiles();
    for (int x = 0; x < i; ++x) {
        for (int y = 0; y < i; ++y) {
            for (int z = 0; z < MaxLevels; ++z) {
                WorldChunkSquare *sq = getGridSquare(x + getWorldXMinTiles(),
                                                     y + getWorldYMinTiles(), z);
                cell->setCacheGridSquareLocal(x, y, z, sq);
            }
        }
    }
#endif
}

int WorldChunkMap::getWorldXMin()
{
    return (WorldX - (ChunkGridWidth / 2));
}

int WorldChunkMap::getWorldYMin()
{
    return (WorldY - (ChunkGridWidth / 2));
}

int WorldChunkMap::getWorldXMinTiles()
{
    if (XMinTiles != -1) return XMinTiles;
    XMinTiles = (getWorldXMin() * TilesPerChunk);
    return XMinTiles;
}

int WorldChunkMap::getWorldYMinTiles()
{
    if (YMinTiles != -1) return YMinTiles;
    YMinTiles = (getWorldYMin() * TilesPerChunk);
    return YMinTiles;
}

void WorldChunkMap::setCenter(int x, int y)
{
    int wx = x / TilesPerChunk;
    int wy = y / TilesPerChunk;
    wx = qBound(ChunkGridWidth / 2, wx, (mWorld->width() * 300) / TilesPerChunk - ChunkGridWidth / 2);
    wy = qBound(ChunkGridWidth / 2, wy, (mWorld->height() * 300) / TilesPerChunk - ChunkGridWidth / 2);
//    qDebug() << "WorldChunkMap::setCenter x,y=" << x << "," << y << " wx,wy=" << wx << "," << wy;
    if (wx != WorldX || wy != WorldY) {
        QRegion current = QRect(getWorldXMin(), getWorldYMin(),
                                ChunkGridWidth, ChunkGridWidth);
        QRegion updated = QRect(wx - ChunkGridWidth / 2, wy - ChunkGridWidth / 2,
                                ChunkGridWidth, ChunkGridWidth);

        // Discard old chunks.
        foreach (QRect r, (current - updated).rects()) {
            for (int x = r.left(); x <= r.right(); x++) {
                for (int y = r.top(); y <= r.bottom(); y++) {
                    if (WorldChunk *c = getChunk(x - getWorldXMin(), y - getWorldYMin())) {
                        c->Save(false);
                        setChunk(x - getWorldXMin(), y - getWorldYMin(), 0);
                        c->discard();
                    }
                }
            }
        }

        // Shift preserved chunks.
        QVector<WorldChunk*> preserved;
        foreach (QRect r, (current & updated).rects()) {
            for (int x = r.left(); x <= r.right(); x++) {
                for (int y = r.top(); y <= r.bottom(); y++) {
                    if (WorldChunk *c = getChunk(x - getWorldXMin(), y - getWorldYMin())) {
                        setChunk(x - getWorldXMin(), y - getWorldYMin(), 0);
                        preserved += c;
                    }
                }
            }
        }

        WorldX = wx;
        WorldY = wy;
        XMinTiles = YMinTiles = -1;

        foreach (WorldChunk *c, preserved) {
            setChunk(c->wx - getWorldXMin(), c->wy - getWorldYMin(), c);
        }

        // Load new chunks;
        foreach (QRect r, (updated - current).rects()) {
            for (int x = r.left(); x <= r.right(); x++) {
                for (int y = r.top(); y <= r.bottom(); y++) {
//                    qDebug() << "Load chunk" << x << "," << y;
                    LoadChunkForLater(x, y, x - getWorldXMin(), y - getWorldYMin());
                }
            }
        }

        UpdateCellCache();
#if 0
        for (int x = 0; x < Chunks.size(); x++) {
            for (int y = 0; y < Chunks[x].size(); y++) {
                if (IsoChunk *chunk = Chunks[x][y]) {
                    if (!mScene->mHeadersExamined.contains(chunk->lotheader)) {
                        mScene->mHeadersExamined += chunk->lotheader;
                        foreach (QString tileName, chunk->lotheader->tilesUsed) {
                            if (!mScene->mTileByName.contains(tileName)) {
                                if (Tile *tile = BuildingEditor::BuildingTilesMgr::instance()->tileFor(tileName)) {
                                    mScene->mTileByName[tileName] = tile;
                                }
                            }
                        }
                    }
                }
            }
        }

        mScene->setMaxLevel(mWorld->CurrentCell->MaxHeight);
#endif
    }
}
