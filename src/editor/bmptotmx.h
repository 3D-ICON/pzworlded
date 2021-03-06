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

#ifndef BMPTOTMX_H
#define BMPTOTMX_H

#include <QImage>
#include <QMap>
#include <QObject>
#include <QStringList>

namespace Tiled {
class BmpAlias;
class BmpBlend;
class BmpRule;
class Tile;
}

class WorldBMP;
class WorldCell;
class WorldDocument;

class BMPToTMXImages
{
public:
    QString mPath;
    QImage mBmp;
    QImage mBmpVeg;
    QRect mBounds; // cells covered
};

class BMPToTMX : public QObject
{
    Q_OBJECT
public:
    static BMPToTMX *instance();
    static void deleteInstance();

    enum GenerateMode {
        GenerateAll,
        GenerateSelected
    };

    bool generateWorld(WorldDocument *worldDoc, GenerateMode mode);
    bool generateCell(WorldCell *cell);

    QString errorString() const { return mError; }

    static QStringList supportedImageFormats();

    BMPToTMXImages *getImages(const QString &path, const QPoint &origin,
                              QImage::Format format = QImage::Format_ARGB32);
    QSize validateImages(const QString &path);

    void assignMapsToCells(WorldDocument *worldDoc, GenerateMode mode);

    QString defaultRulesFile() const;
    QString defaultBlendsFile() const;
    QString defaultMapBaseXMLFile() const;

private:
    bool shouldGenerateCell(WorldCell *cell, int &bmpIndex);

    QImage loadImage(const QString &path, const QString &suffix = QString(),
                     QImage::Format format = QImage::Format_ARGB32);
    bool LoadBaseXML();
    bool LoadRules();
    bool LoadBlends();

    void AddRule(Tiled::BmpRule *rule);

    bool WriteMap(WorldCell *cell, int bmpIndex);
    bool UpdateMap(WorldCell *cell, int bmpIndex);

    Tiled::Tile *getTileFromTileName(const QString &tileName);

    void assignMapToCell(WorldCell *cell);

    QString tmxNameForCell(WorldCell *cell, WorldBMP *bmp);

    void reportUnknownColors();

private:
    Q_DISABLE_COPY(BMPToTMX)

    explicit BMPToTMX(QObject *parent = 0);
    ~BMPToTMX();

    static BMPToTMX *mInstance;

    WorldDocument *mWorldDoc;
    QList<BMPToTMXImages*> mImages;

    QString mRuleFileName;
    QList<Tiled::BmpAlias*> mAliases;
    QMap<QString,Tiled::BmpAlias*> mAliasByName;
    QList<Tiled::BmpRule*> mRules;
    QMap<QRgb,QList<Tiled::BmpRule*> > mRulesByColor0;
    QMap<QRgb,QList<Tiled::BmpRule*> > mRulesByColor1;

    class LayerInfo
    {
    public:
        enum Type {
            Tile,
            Object
        };

        LayerInfo(const QString &name, Type type) :
            mName(name),
            mType(type)
        {}

        QString mName;
        Type mType;
    };
    QList<LayerInfo> mLayers;

    QString mBlendFileName;
    QList<Tiled::BmpBlend*> mBlends;

    QString mError;

    struct UnknownColor {
        QRgb rgb;
        QList<QPoint> xy;
    };
    QMap<QString,QMap<QRgb,UnknownColor> > mUnknownColors;
    QMap<QString,QMap<QRgb,UnknownColor> > mUnknownVegColors;

    QStringList mNewFiles;
};

#endif // BMPTOTMX_H
