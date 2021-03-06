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

#ifndef OBJECTGROUPSDIALOG_H
#define OBJECTGROUPSDIALOG_H

#include <QDialog>
#include <QMap>

class WorldDocument;
class WorldObjectGroup;

class QListWidgetItem;
namespace Ui {
class ObjectGroupsDialog;
}

class ObjectGroupsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ObjectGroupsDialog(WorldDocument *worldDoc, QWidget *parent = 0);

private slots:
    void selectionChanged();
    void add();
    void update();
    void remove();
    void synchButtons();

    void moveGroupUp();
    void moveGroupDown();

    void colorChanged(const QColor &color);

    void typeChanged(int index);

private:
    void setList();
    void clearUI();

private:
    Ui::ObjectGroupsDialog *ui;
    WorldDocument *mWorldDoc;
    WorldObjectGroup *mObjGroup;
    QListWidgetItem *mItem;
    int mSelectedRow;
    bool mSynching;
    QMap<QListWidgetItem*,WorldObjectGroup*> mItemToGroup;
};

#endif // OBJECTGROUPSDIALOG_H
