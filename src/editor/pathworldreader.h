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

#ifndef PATHWORLDREADER_H
#define PATHWORLDREADER_H

#include <QString>

class PathWorld;

class QIODevice;

class PathWorldReaderPrivate;

class PathWorldReader
{
public:
    PathWorldReader();
    ~PathWorldReader();

    PathWorld *readWorld(QIODevice *device, const QString &path = QString());
    PathWorld *readWorld(const QString &fileName);

    QString errorString() const;

private:
    friend class PathWorldReaderPrivate;
    PathWorldReaderPrivate *d;
};

#endif // PATHWORLDREADER_H