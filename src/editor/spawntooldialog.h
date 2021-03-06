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

#ifndef SPAWNTOOLDIALOG_H
#define SPAWNTOOLDIALOG_H

#include "singleton.h"

#include <QDialog>

class CellDocument;
class PropertyEnum;
class PropertyHolder;
class WorldCellObject;
class WorldDocument;

class QListWidgetItem;

namespace Ui {
class SpawnToolDialog;
}

class SpawnToolDialog : public QDialog, public Singleton<SpawnToolDialog>
{
    Q_OBJECT
    
public:
    explicit SpawnToolDialog(QWidget *parent = 0);
    ~SpawnToolDialog();

    void setVisible(bool visible);

    void setDocument(CellDocument *doc);
    WorldDocument *worldDocument();

private slots:
    void selectedObjectsChanged();

    void propertyEnumAboutToBeRemoved(int index);
    void propertyEnumChoicesChanged(PropertyEnum *pe);
    void checkProfessionsEnum();

    void propertiesChanged(PropertyHolder *ph);

    void addItem();
    void removeItem();

    void itemChanged(QListWidgetItem *item);
    void currentRowChanged(int row);
    
private:
    void setList();
    void toObjects();
    QList<WorldCellObject*> selectedSpawnPoints();
    QStringList professions();
    PropertyEnum *professionsEnum();
    QString makeNameUnique(const QString &name, int ignore);

private:
    Ui::SpawnToolDialog *ui;
    CellDocument *mDocument;
};

#endif // SPAWNTOOLDIALOG_H
