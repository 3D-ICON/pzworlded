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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "fromtodialog.h"
#include "bmptotmx.h"
#include "bmptotmxdialog.h"
#include "celldocument.h"
#include "cellscene.h"
#include "cellview.h"
#include "clipboard.h"
#include "copypastedialog.h"
#include "documentmanager.h"
#include "generatelotsdialog.h"
#include "layersdock.h"
#include "lotsdock.h"
#include "lotfilesmanager.h"
#include "lotpackwindow.h"
#include "mapcomposite.h"
#include "mapimagemanager.h"
#include "mapmanager.h"
#include "mapsdock.h"
#include "newworlddialog.h"
#include "objectsdock.h"
#include "objectgroupsdialog.h"
#include "objecttypesdialog.h"
#include "preferences.h"
#include "preferencesdialog.h"
#include "progress.h"
#include "propertiesdock.h"
#include "propertydefinitionsdialog.h"
#include "resizeworlddialog.h"
#include "roadsdock.h"
#include "scenetools.h"
#include "templatesdialog.h"
#include "tilemetainfomgr.h"
#include "tilesetmanager.h"
#include "toolmanager.h"
#include "undodock.h"
#include "world.h"
#include "worlddocument.h"
#include "worldreader.h"
#include "worldscene.h"
#include "worldview.h"
#include "worldwriter.h"
#include "zoomable.h"

#include "layer.h"
#include "mapobject.h"
#include "objectgroup.h"
#include "tileset.h"

#include <QtCore/qmath.h>
#include <QBuffer>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QScrollBar>
#include <QUndoGroup>
#include <QUndoStack>

#ifdef Q_OS_WIN
// Hmmmm.  libtiled.dll defines the Properties class as so:
// class TILEDSHARED_EXPORT Properties : public QMap<QString,QString>
// Suddenly I'm getting a 'multiply-defined symbol' error.
// I found the solution here:
// http://www.archivum.info/qt-interest@trolltech.com/2005-12/00242/RE-Linker-Problem-while-using-QMap.html
template class __declspec(dllimport) QMap<QString, QString>;
#endif

using namespace Tiled;
using namespace Tiled::Internal;

MainWindow *MainWindow::mInstance = 0;

MainWindow *MainWindow::instance()
{
    return mInstance;
}

#include "basepathrenderer.h"
#include "isopathscene.h"
#include "luatiled.h"
#include "osmfile.h"
#include "orthopathscene.h"
#include "pathview.h"
#include "path.h"
#include "pathdocument.h"
static void testPathDocument()
{
    World *newWorld = new World(30, 30);

#if 1
    PROGRESS progress(QLatin1String("Reading OSM data"));
    OSM::File osm;
    QString f = QLatin1String("C:\\Programming\\OpenStreetMap\\Vancouver2.osm");
    if (osm.read(f)) {
        WorldPath::Layer *pathLayer = new WorldPath::Layer();
        int i = 0;
        int zoom = 15;
        int TILE_SIZE = 512 + 64;
        int worldGridX1 = qFloor(osm.minProjected().x * (1U << zoom)) * TILE_SIZE;
        int worldGridY1 = qFloor(osm.minProjected().y * (1U << zoom)) * TILE_SIZE;
        double worldInPixels = (1u << zoom) * TILE_SIZE; // size of world in pixels

        foreach (OSM::Node *n, osm.nodes())
            pathLayer->insertNode(i++, new WorldPath::Node(n->id,
                                                           n->pc.x * worldInPixels - worldGridX1,
                                                           n->pc.y * worldInPixels - worldGridY1));

        i = 0;
        foreach (OSM::Way *w, osm.ways()) {
            WorldPath::Path *path = new WorldPath::Path(w->id);
            path->tags = w->tags;
            int j = 0;
            foreach (OSM::Node *nd, w->nodes)
                path->insertNode(j++, pathLayer->node(nd->id));
            pathLayer->insertPath(i++, path);
        }
        newWorld->insertLayer(0, pathLayer);
    }

    foreach (WorldPath::Layer *layer, newWorld->layers()) {
        foreach (WorldPath::Path *path, layer->paths()) {
            QString script;
            if (path->tags.contains(QLatin1String("landuse"))) {
                QString v = path->tags[QLatin1String("landuse")];
                if (v == QLatin1String("forest") || v == QLatin1String("wood"))
                    ;
                else if (v == QLatin1String("park"))
                    ;
                else if (v == QLatin1String("grass"))
                    ;
            } else if (path->tags.contains(QLatin1String("natural"))) {
                QString v = path->tags[QLatin1String("natural")];
                if (v == QLatin1String("water"))
                    ;

            } else if (path->tags.contains(QLatin1String("leisure"))) {
                QString v = path->tags[QLatin1String("leisure")];
                if (v == QLatin1String("park"))
                    script = QLatin1String("C:/Programming/Tiled/PZWorldEd/park.lua");
            }
            if (path->tags.contains(QLatin1String("highway"))) {
                qreal width;
                QString v = path->tags[QLatin1String("highway")];
                if (v == QLatin1String("residential")) width = 6; /// #1
                else if (v == QLatin1String("pedestrian")) width = 5;
                else if (v == QLatin1String("secondary")) width = 5;
                else if (v == QLatin1String("secondary_link")) width = 5;
                else if (v == QLatin1String("tertiary")) width = 4; /// #3
                else if (v == QLatin1String("tertiary_link")) width = 4;
                else if (v == QLatin1String("bridleway")) width = 4;
                else if (v == QLatin1String("private")) width = 4;
                else if (v == QLatin1String("service")) width = 6/2; /// #2
                else if (v == QLatin1String("path")) width = 3; /// #2
                else continue;
                script = QLatin1String("C:/Programming/Tiled/PZWorldEd/road.lua");
            }

            if (!script.isEmpty()) {
                WorldScript *ws = new WorldScript;
                ws->mFileName = script;
                ws->mPaths += path;
                newWorld->insertScript(newWorld->scriptCount(), ws);
            }

            if (path->tags.contains(QLatin1String("building")))
            {
                WorldScript *ws = new WorldScript;
                ws->mFileName = QLatin1String("C:/Programming/Tiled/PZWorldEd/place-lot.lua");
                ws->mParams[QLatin1String("mapfile")] = QLatin1String("C:\\Users\\Tim\\Desktop\\ProjectZomboid\\Tools\\Buildings\\tiny-house.tbx");
                ws->mParams[QLatin1String("pos")] = QString::fromLatin1("%1 %2").arg(path->bounds().center().x()).arg(path->bounds().center().y());
                newWorld->insertScript(newWorld->scriptCount(), ws);
            }
        }
    }


    progress.update(QLatin1String("Running scripts (region)"));

    foreach (WorldScript *ws, newWorld->scripts()) {
        Lua::LuaScript ls(newWorld, ws);
        if (ls.runFunction("region")) {
            QRegion rgn;
            if (ls.getResultRegion(rgn))
                ws->mRegion = rgn;
        }
    }

    progress.update(QLatin1String("Loading tilesets"));

    newWorld->insertTileLayer(0, new WorldTileLayer(newWorld, QLatin1String("0_Floor")));

    foreach (Tileset *ts, TileMetaInfoMgr::instance()->tilesets()) {
        WorldTileset *wts = new WorldTileset(ts->name(), ts->columnCount(), ts->tileCount() / ts->columnCount());
        for (int i = 0; i < ts->tileCount(); i++)
            wts->tileAt(i)->mTiledTile = ts->tileAt(i); // HACK for convenience
        newWorld->insertTileset(newWorld->tilesetCount(), wts);
    }
    TileMetaInfoMgr::instance()->loadTilesets();

#else
    WorldPath::Node *n1 = new WorldPath::Node(1, 10, 10);
    WorldPath::Node *n2 = new WorldPath::Node(2, 10, 20);
    WorldPath::Node *n3 = new WorldPath::Node(3, 20, 20);
    WorldPath::Path *p1 = new WorldPath::Path(1);
    p1->insertNode(0, n1); p1->insertNode(1, n2); p1->insertNode(2, n3);
    WorldPath::Layer *pathLayer = new WorldPath::Layer();
    pathLayer->insertNode(0, n1); pathLayer->insertNode(1, n2); pathLayer->insertNode(2, n3);
    pathLayer->insertPath(0, p1);
    newWorld->insertLayer(0, pathLayer);
#endif

    PathDocument *newDoc = new PathDocument(newWorld);
    DocumentManager::instance()->addDocument(newDoc);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mLayersDock(new LayersDock(this))
    , mLotsDock(new LotsDock(this))
    , mMapsDock(new MapsDock(this))
    , mObjectsDock(new ObjectsDock(this))
    , mPropertiesDock(new PropertiesDock(this))
    , mRoadsDock(new RoadsDock(this))
    , mCurrentDocument(0)
    , mCurrentLevelMenu(new QMenu(this))
    , mObjectGroupMenu(new QMenu(this))
    , mZoomable(0)
    , mLotPackWindow(0)
{
    ui->setupUi(this);

    mInstance = this;

    Preferences *prefs = Preferences::instance();

    setStatusBar(0);
    mZoomComboBox = ui->zoomComboBox;

    QString coordString = QString(QLatin1String("%1,%2")).arg(300).arg(300);
    int width = ui->coordinatesLabel->fontMetrics().width(coordString);
    ui->coordinatesLabel->setMinimumWidth(width + 8);

    ui->actionSave->setShortcuts(QKeySequence::Save);
    ui->actionSaveAs->setShortcuts(QKeySequence::SaveAs);
    ui->actionClose->setShortcuts(QKeySequence::Close);
    ui->actionQuit->setShortcuts(QKeySequence::Quit);

    ui->actionCopy->setShortcuts(QKeySequence::Copy);
    ui->actionPaste->setShortcuts(QKeySequence::Paste);

    ui->actionSnapToGrid->setChecked(prefs->snapToGrid());
    ui->actionShowCoordinates->setChecked(prefs->showCoordinates());
    ui->actionShowGrid->setChecked(prefs->showWorldGrid());
    ui->actionShowMiniMap->setChecked(prefs->showMiniMap());
    ui->actionShowObjectNames->setChecked(prefs->showObjectNames());
    ui->actionShowBMP->setChecked(prefs->showBMPs());
    ui->actionHighlightCurrentLevel->setChecked(prefs->highlightCurrentLevel());

    // Make sure Ctrl+= also works for zooming in
    QList<QKeySequence> keys = QKeySequence::keyBindings(QKeySequence::ZoomIn);
    keys += QKeySequence(tr("Ctrl+="));
    keys += QKeySequence(tr("+"));
    keys += QKeySequence(tr("="));
    ui->actionZoomIn->setShortcuts(keys);
    keys = QKeySequence::keyBindings(QKeySequence::ZoomOut);
    keys += QKeySequence(tr("-"));
    ui->actionZoomOut->setShortcuts(keys);

    QUndoGroup *undoGroup = docman()->undoGroup();
    QAction *undoAction = undoGroup->createUndoAction(this, tr("Undo"));
    QAction *redoAction = undoGroup->createRedoAction(this, tr("Redo"));
    redoAction->setPriority(QAction::LowPriority);
    undoAction->setIconText(tr("Undo"));
    undoAction->setShortcuts(QKeySequence::Undo);
    redoAction->setIconText(tr("Redo"));
    redoAction->setShortcuts(QKeySequence::Redo);
    connect(undoGroup, SIGNAL(cleanChanged(bool)), SLOT(updateWindowTitle()));
    QAction *separator = ui->editMenu->actions().first();
    ui->editMenu->insertAction(separator, undoAction);
    ui->editMenu->insertAction(separator, redoAction);

    ui->mainToolBar->addAction(undoAction);
    ui->mainToolBar->addAction(redoAction);
    ui->mainToolBar->addSeparator();

    QIcon newIcon = ui->actionNew->icon();
    QIcon openIcon = ui->actionOpen->icon();
    QIcon saveIcon = ui->actionSave->icon();
    QIcon redoIcon(QLatin1String(":images/16x16/edit-redo.png"));
    QIcon undoIcon(QLatin1String(":images/16x16/edit-undo.png"));
    newIcon.addFile(QLatin1String(":images/24x24/document-new.png"));
    openIcon.addFile(QLatin1String(":images/24x24/document-open.png"));
    saveIcon.addFile(QLatin1String(":images/24x24/document-save.png"));
    redoIcon.addFile(QLatin1String(":images/24x24/edit-redo.png"));
    undoIcon.addFile(QLatin1String(":images/24x24/edit-undo.png"));

    ui->actionNew->setIcon(newIcon);
    ui->actionOpen->setIcon(openIcon);
    ui->actionSave->setIcon(saveIcon);
    undoAction->setIcon(undoIcon);
    redoAction->setIcon(redoIcon);

    mUndoDock = new UndoDock(undoGroup, this);

    ui->menuView->addAction(mLayersDock->toggleViewAction());
    ui->menuView->addAction(mLotsDock->toggleViewAction());
    ui->menuView->addAction(mMapsDock->toggleViewAction());
    ui->menuView->addAction(mObjectsDock->toggleViewAction());
    ui->menuView->addAction(mPropertiesDock->toggleViewAction());
    ui->menuView->addAction(mRoadsDock->toggleViewAction());

    addDockWidget(Qt::LeftDockWidgetArea, mLotsDock);
    addDockWidget(Qt::LeftDockWidgetArea, mObjectsDock);
    addDockWidget(Qt::LeftDockWidgetArea, mRoadsDock);
    addDockWidget(Qt::RightDockWidgetArea, mPropertiesDock);
    addDockWidget(Qt::RightDockWidgetArea, mLayersDock);
    addDockWidget(Qt::RightDockWidgetArea, mMapsDock);
    tabifyDockWidget(mPropertiesDock, mLayersDock);
    tabifyDockWidget(mLayersDock, mMapsDock);
    tabifyDockWidget(mObjectsDock, mLotsDock);

    addDockWidget(Qt::RightDockWidgetArea, mUndoDock);

    connect(ui->actionNew, SIGNAL(triggered()), SLOT(newWorld()));
    connect(ui->actionOpen, SIGNAL(triggered()), SLOT(openFile()));
    connect(ui->actionEditCell, SIGNAL(triggered()), SLOT(editCell()));
    connect(ui->actionSave, SIGNAL(triggered()), SLOT(saveFile()));
    connect(ui->actionSaveAs, SIGNAL(triggered()), SLOT(saveFileAs()));
    connect(ui->actionClose, SIGNAL(triggered()), SLOT(closeFile()));
    connect(ui->actionCloseAll, SIGNAL(triggered()), SLOT(closeAllFiles()));
    connect(ui->actionGenerateLotsAll, SIGNAL(triggered()),
            SLOT(generateLotsAll()));
    connect(ui->actionGenerateLotsSelected, SIGNAL(triggered()),
            SLOT(generateLotsSelected()));
    connect(ui->actionBMPToTMXAll, SIGNAL(triggered()),
            SLOT(BMPToTMXAll()));
    connect(ui->actionBMPToTMXSelected, SIGNAL(triggered()),
            SLOT(BMPToTMXSelected()));
    connect(ui->actionFromToAll, SIGNAL(triggered()),
            SLOT(FromToAll()));
    connect(ui->actionFromToSelected, SIGNAL(triggered()),
            SLOT(FromToSelected()));
    connect(ui->actionQuit, SIGNAL(triggered()), SLOT(close()));

    connect(ui->actionCopy, SIGNAL(triggered()), SLOT(copy()));
    connect(ui->actionPaste, SIGNAL(triggered()), SLOT(paste()));
    connect(ui->actionClipboard, SIGNAL(triggered()), SLOT(showClipboard()));

    connect(ui->actionPreferences, SIGNAL(triggered()), SLOT(preferencesDialog()));

    connect(ui->actionResizeWorld, SIGNAL(triggered()), SLOT(resizeWorld()));
    connect(ui->actionObjectGroups, SIGNAL(triggered()), SLOT(objectGroupsDialog()));
    connect(ui->actionObjectTypes, SIGNAL(triggered()), SLOT(objectTypesDialog()));
    connect(ui->actionProperties, SIGNAL(triggered()), SLOT(properyDefinitionsDialog()));
    connect(ui->actionTemplates, SIGNAL(triggered()), SLOT(templatesDialog()));
    connect(ui->actionRemoveRoad, SIGNAL(triggered()), SLOT(removeRoad()));
    connect(ui->actionRemoveBMP, SIGNAL(triggered()), SLOT(removeBMP()));

    connect(ui->actionRemoveLot, SIGNAL(triggered()), SLOT(removeLot()));
    connect(ui->actionRemoveObject, SIGNAL(triggered()), SLOT(removeObject()));
    connect(ui->actionExtractLots, SIGNAL(triggered()), SLOT(extractLots()));
    connect(ui->actionExtractObjects, SIGNAL(triggered()), SLOT(extractObjects()));
    connect(ui->actionClearCell, SIGNAL(triggered()), SLOT(clearCells()));
    connect(ui->actionClearMapOnly, SIGNAL(triggered()), SLOT(clearMapOnly()));

    connect(ui->actionSnapToGrid, SIGNAL(toggled(bool)), prefs, SLOT(setSnapToGrid(bool)));
    connect(ui->actionShowCoordinates, SIGNAL(toggled(bool)), prefs, SLOT(setShowCoordinates(bool)));
    connect(ui->actionShowGrid, SIGNAL(toggled(bool)), SLOT(setShowGrid(bool)));
    connect(ui->actionShowMiniMap, SIGNAL(toggled(bool)), prefs, SLOT(setShowMiniMap(bool)));
    connect(ui->actionShowObjectNames, SIGNAL(toggled(bool)), prefs, SLOT(setShowObjectNames(bool)));
    connect(ui->actionShowBMP, SIGNAL(toggled(bool)), prefs, SLOT(setShowBMPs(bool)));
    connect(ui->actionHighlightCurrentLevel, SIGNAL(toggled(bool)), prefs, SLOT(setHighlightCurrentLevel(bool)));
    connect(ui->actionLevelAbove, SIGNAL(triggered()), SLOT(selectLevelAbove()));
    connect(ui->actionLevelBelow, SIGNAL(triggered()), SLOT(selectLevelBelow()));
    connect(ui->actionZoomIn, SIGNAL(triggered()), SLOT(zoomIn()));
    connect(ui->actionZoomOut, SIGNAL(triggered()), SLOT(zoomOut()));
    connect(ui->actionZoomNormal, SIGNAL(triggered()), SLOT(zoomNormal()));
    connect(ui->actionSwitchOrthoIso, SIGNAL(triggered()), SLOT(switchOrthoIso()));

    connect(ui->actionLotPackViewer, SIGNAL(triggered()), SLOT(lotpackviewer()));
    connect(ui->actionWorldGen, SIGNAL(triggered()), SLOT(WorldGen()));
    connect(ui->actionLuaConsole, SIGNAL(triggered()), SLOT(LuaConsole()));

    connect(ui->actionAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    connect(docman(), SIGNAL(documentAdded(Document*)), SLOT(documentAdded(Document*)));
    connect(docman(), SIGNAL(documentAboutToClose(int,Document*)), SLOT(documentAboutToClose(int,Document*)));
    connect(docman(), SIGNAL(currentDocumentChanged(Document*)), SLOT(currentDocumentChanged(Document*)));

    ToolManager *toolManager = ToolManager::instance();
    toolManager->registerTool(WorldCellTool::instance());
    toolManager->registerTool(PasteCellsTool::instance());
    toolManager->registerTool(WorldSelectMoveRoadTool::instance());
    toolManager->registerTool(WorldCreateRoadTool::instance());
    toolManager->registerTool(WorldEditRoadTool::instance());
    toolManager->registerTool(WorldBMPTool::instance());
    toolManager->addSeparator();
    toolManager->registerTool(SubMapTool::instance());
    toolManager->registerTool(ObjectTool::instance());
    toolManager->registerTool(CreateObjectTool::instance());
    toolManager->registerTool(CellSelectMoveRoadTool::instance());
    toolManager->registerTool(CellCreateRoadTool::instance());
    toolManager->registerTool(CellEditRoadTool::instance());
    addToolBar(toolManager->toolBar());

    ui->currentLevelButton->setMenu(mCurrentLevelMenu);
    connect(mCurrentLevelMenu, SIGNAL(aboutToShow()), SLOT(aboutToShowCurrentLevelMenu()));
    connect(mCurrentLevelMenu, SIGNAL(triggered(QAction*)), SLOT(currentLevelMenuTriggered(QAction*)));

    ui->objectGroupButton->setMenu(mObjectGroupMenu);
    connect(mObjectGroupMenu, SIGNAL(aboutToShow()),
            SLOT(aboutToShowObjGrpMenu()));
    connect(mObjectGroupMenu, SIGNAL(triggered(QAction*)),
            SLOT(objGrpMenuTriggered(QAction*)));

    ui->documentTabWidget->clear(); // TODO: remove tabs from .ui
    ui->documentTabWidget->setDocumentMode(true);
    ui->documentTabWidget->setTabsClosable(true);

    connect(ui->documentTabWidget, SIGNAL(currentChanged(int)),
            SLOT(currentDocumentTabChanged(int)));
    connect(ui->documentTabWidget, SIGNAL(tabCloseRequested(int)),
            SLOT(documentCloseRequested(int)));

    enableDeveloperFeatures();

    Progress::instance()->setMainWindow(this);

    mViewHint.valid = false;

    updateActions();

    readSettings();
}

MainWindow::~MainWindow()
{
#if 1
    // MapComposite's destructor calls MapManager::removeReferenceToMap().
    // But the MapComposite's aren't deleted till the base constructor has
    // run, which causes MapManager to be *recreated* and recreate its threads.
    // I think this leads to the application not terminating promptly.
    ToolManager::instance()->toolBar()->setParent(0);
#else
    DocumentManager::deleteInstance();
    ToolManager::deleteInstance();
    Preferences::deleteInstance();
    MapImageManager::deleteInstance();
    MapManager::deleteInstance();
    TileMetaInfoMgr::deleteInstance();
    TilesetManager::deleteInstance();
#endif
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();

    if (mLotPackWindow)
        mLotPackWindow->close();

    if (confirmAllSave())
        event->accept();
    else
        event->ignore();
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    switch (event->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        retranslateUi();
        break;
    default:
        break;
    }
}

void MainWindow::retranslateUi()
{
    setWindowTitle(tr("PZWorldEd"));
}

void MainWindow::newWorld()
{
    testPathDocument(); return;

    NewWorldDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    QSize size = dialog.worldSize();

    World *newWorld = new World(size.width(), size.height());
    WorldDocument *newDoc = new WorldDocument(newWorld);
    docman()->addDocument(newDoc);
}

void MainWindow::editCell()
{
    Document *doc = docman()->currentDocument();
    if (WorldDocument *worldDoc = doc->asWorldDocument()) {
        foreach (WorldCell *cell, worldDoc->selectedCells())
            worldDoc->editCell(cell);
    }
}

void MainWindow::setShowGrid(bool show)
{
    if (mCurrentDocument && mCurrentDocument->isWorldDocument())
        Preferences::instance()->setShowWorldGrid(show);
    else if (mCurrentDocument && mCurrentDocument->isCellDocument())
        Preferences::instance()->setShowCellGrid(show);
}

void MainWindow::documentAdded(Document *doc)
{
    if (CellDocument *cellDoc = doc->asCellDocument()) {
        CellView *view = new CellView(this);
        CellScene *scene = new CellScene(view);
        view->setScene(scene);
        scene->setDocument(cellDoc);
        // Handle failure to load the map.
        // Currently this never happens as a placeholder map
        // will be used in case of failure.
        if (scene->mapComposite() == 0) {
            delete scene;
            delete view;
            docman()->setFailedToAdd();
            return;
        }
        doc->setView(view);
        cellDoc->setScene(scene);

        int pos = docman()->documents().indexOf(doc);
        ui->documentTabWidget->insertTab(pos, view,
            tr("Cell %1,%2").arg(cellDoc->cell()->x()).arg(cellDoc->cell()->y()));
        ui->documentTabWidget->setTabToolTip(pos, doc->fileName());

        if (mViewHint.valid) {
            view->zoomable()->setScale(mViewHint.scale);
            view->centerOn(mViewHint.scrollX, mViewHint.scrollY);
        }
        else
            view->centerOn(scene->sceneRect().center());
    }
    if (WorldDocument *worldDoc = doc->asWorldDocument()) {
        WorldView *view = new WorldView(this);
        WorldScene *scene = new WorldScene(worldDoc, view);
        view->setScene(scene);
        doc->setView(view);

        int pos = docman()->documents().indexOf(doc);
        ui->documentTabWidget->insertTab(pos, view, tr("The World"));
        ui->documentTabWidget->setTabToolTip(pos, doc->fileName());

        if (mViewHint.valid) {
            view->zoomable()->setScale(mViewHint.scale);
            view->centerOn(mViewHint.scrollX, mViewHint.scrollY);
        } else
            view->centerOn(scene->cellToPixelCoords(0, 0));
    }
    if (PathDocument *pathDoc = doc->asPathDocument()) {
        PathView *view = new PathView(pathDoc, this);
        view->switchToOrtho();
        doc->setView(view);

        int pos = docman()->documents().indexOf(doc);
        ui->documentTabWidget->insertTab(pos, view, tr("The World"));
        ui->documentTabWidget->setTabToolTip(pos, doc->fileName());
        view->centerOn(view->scene()->renderer()->toScene(QPoint()));
    }
}

void MainWindow::documentAboutToClose(int index, Document *doc)
{
    // If there was an error adding the document (such as failure to
    // load the map of a cell) then there won't be a tab yet.

    // At this point, the document is not in the DocumentManager's list of documents.
    // Removing the current tab will cause another tab to be selected and
    // the current document to change.
    ui->documentTabWidget->removeTab(index);

    // Delete the QGraphicsView (and QGraphicsScene owned by the view)
    delete doc->view();
}

void MainWindow::currentDocumentTabChanged(int tabIndex)
{
    docman()->setCurrentDocument(tabIndex);
}

void MainWindow::currentDocumentChanged(Document *doc)
{
    if (mCurrentDocument) {
        mCurrentDocument->disconnect(this);
        mZoomable->connectToComboBox(0);
        mZoomable->disconnect(this);
        mZoomable = 0;
    }

    mCurrentDocument = doc;

    if (mCurrentDocument) {
        if (CellDocument *cellDoc = doc->asCellDocument()) {
            connect(cellDoc, SIGNAL(currentLevelChanged(int)), SLOT(updateActions()));
            connect(cellDoc, SIGNAL(selectedLotsChanged()), SLOT(updateActions()));
            connect(cellDoc, SIGNAL(selectedObjectsChanged()), SLOT(updateActions()));
            connect(cellDoc, SIGNAL(currentObjectGroupChanged(WorldObjectGroup*)),
                    SLOT(updateActions()));
            connect(cellDoc->view(), SIGNAL(statusBarCoordinatesChanged(int,int)),
                    SLOT(setStatusBarCoords(int,int)));
            connect(cellDoc->worldDocument(),
                    SIGNAL(objectGroupNameChanged(WorldObjectGroup*)),
                    SLOT(updateActions()));
            connect(cellDoc->worldDocument(), SIGNAL(selectedRoadsChanged()),
                    SLOT(updateActions()));
        }

        if (WorldDocument *worldDoc = doc->asWorldDocument()) {
            connect(worldDoc, SIGNAL(selectedCellsChanged()), SLOT(updateActions()));
            connect(worldDoc, SIGNAL(selectedLotsChanged()), SLOT(updateActions()));
            connect(worldDoc, SIGNAL(selectedObjectsChanged()), SLOT(updateActions()));
            connect(worldDoc->view(), SIGNAL(statusBarCoordinatesChanged(int,int)),
                    SLOT(setStatusBarCoords(int,int)));
            connect(worldDoc, SIGNAL(selectedRoadsChanged()),
                    SLOT(updateActions()));
            connect(worldDoc, SIGNAL(selectedBMPsChanged()),
                    SLOT(updateActions()));
        }

        mLotsDock->setDocument(doc);
        mObjectsDock->setDocument(doc);
        mRoadsDock->setDocument(doc);

        mZoomable = mCurrentDocument->view()->zoomable();
        mZoomable->connectToComboBox(mZoomComboBox);
        connect(mZoomable, SIGNAL(scaleChanged(qreal)), SLOT(updateZoom()));

        // May be a WorldDocument, that's ok
        CellDocument *cellDoc = mCurrentDocument->asCellDocument();
        mLayersDock->setCellDocument(cellDoc);
    } else {
        mLayersDock->setCellDocument(0);
        mLotsDock->clearDocument();
        mObjectsDock->clearDocument();
        mRoadsDock->clearDocument();
    }

    ToolManager::instance()->setScene(doc ? doc->view()->scene() : 0);
    mPropertiesDock->setDocument(doc);

    ui->documentTabWidget->setCurrentIndex(docman()->indexOf(doc));

    updateActions();
    updateWindowTitle();
}

void MainWindow::documentCloseRequested(int tabIndex)
{
    Document *doc = docman()->documentAt(tabIndex);
    if (doc->isModified()) {
        docman()->setCurrentDocument(tabIndex);
        if (!confirmSave())
            return;
    }
    docman()->closeDocument(tabIndex);
}

void MainWindow::selectLevelAbove()
{
    if (CellDocument *cellDoc = mCurrentDocument->asCellDocument()) {
        int level = cellDoc->currentLevel();
        if (level < cellDoc->scene()->mapComposite()->maxLevel())
            cellDoc->setCurrentLevel(level + 1);
    }
}

void MainWindow::selectLevelBelow()
{
    if (CellDocument *cellDoc = mCurrentDocument->asCellDocument()) {
        int level = cellDoc->currentLevel();
        if (level > 0)
            cellDoc->setCurrentLevel(level - 1);
    }
}

void MainWindow::zoomIn()
{
    if (mZoomable)
        mZoomable->zoomIn();
}

void MainWindow::zoomOut()
{
    if (mZoomable)
        mZoomable->zoomOut();
}

void MainWindow::zoomNormal()
{
    if (mZoomable)
        mZoomable->resetZoom();
}

DocumentManager *MainWindow::docman() const
{
    return DocumentManager::instance();
}

void MainWindow::openFile()
{
    QString filter = tr("All Files (*)");
    filter += QLatin1String(";;");

    QString selectedFilter = tr("PZWorldEd world files (*.pzw)");
    filter += selectedFilter;

    QStringList fileNames =
            QFileDialog::getOpenFileNames(this, tr("Open World"),
                                          Preferences::instance()->openFileDirectory(),
                                          filter, &selectedFilter);
    if (fileNames.isEmpty())
        return;

    Preferences::instance()->setOpenFileDirectory(QFileInfo(fileNames[0]).absolutePath());

    foreach (const QString &fileName, fileNames)
        openFile(fileName/*, mapReader*/);
}

bool MainWindow::openFile(const QString &fileName)
{
    if (fileName.isEmpty())
        return false;

    // Select existing document if this file is already open
    int documentIndex = docman()->findDocument(fileName);
    if (documentIndex != -1) {
        ui->documentTabWidget->setCurrentIndex(documentIndex);
        return true;
    }

    QFileInfo fileInfo(fileName);
    PROGRESS progress(tr("Reading %1").arg(fileInfo.fileName()));

    WorldReader reader;
    World *world = reader.readWorld(fileName);
    if (!world) {
        QMessageBox::critical(this, tr("Error Reading World"),
                              reader.errorString());
        return false;
    }

    docman()->addDocument(new WorldDocument(world, fileName));
    if (docman()->failedToAdd())
        return false;
//    setRecentFile(fileName);
    return true;
}

void MainWindow::openLastFiles()
{
    PROGRESS progress(tr("Restoring session"));

    // MapImageManager's threads will load in the thumbnail images in the
    // background.  But defer updating the display with those images until
    // all the documents are loaded.
    MapImageManagerDeferral defer;

    mSettings.beginGroup(QLatin1String("openFiles"));

    int count = mSettings.value(QLatin1String("count")).toInt();

    for (int i = 0; i < count; i++) {
        QString key = QString::number(i); // openFiles/N/...
        if (!mSettings.childGroups().contains(key))
            continue;
        mSettings.beginGroup(key);
        QString path = mSettings.value(QLatin1String("file")).toString();

        // Restore camera to the previous position
        qreal scale = mSettings.value(QLatin1String("scale")).toDouble();
        const int hor = mSettings.value(QLatin1String("scrollX")).toInt();
        const int ver = mSettings.value(QLatin1String("scrollY")).toInt();
        setDocumentViewHint(qMax(scale, 0.06), hor, ver);

        // This "recent file" could be a world or just a cell.
        // We require that the world be already open when editing a cell.
        if (openFile(path)) {
            if (mSettings.contains(QLatin1String("cellX"))) {
                int cellX = mSettings.value(QLatin1String("cellX")).toInt();
                int cellY = mSettings.value(QLatin1String("cellY")).toInt();
                WorldDocument *worldDoc = mCurrentDocument->asWorldDocument();
                if (WorldCell *cell = worldDoc->world()->cellAt(cellX, cellY)) {
                    CellDocument *cellDoc = new CellDocument(worldDoc, cell);
                    docman()->addDocument(cellDoc); // switches document
                    // If the cell map couldn't be loaded, the document
                    // will have been deleted.
                    if (docman()->failedToAdd()) {
                        mSettings.endGroup();
                        continue;
                    }
                    int layerIndex = mSettings.value(QLatin1String("currentLayer")).toInt();
                    if (layerIndex >= 0 && layerIndex < cellDoc->scene()->map()->layerCount())
                        cellDoc->setCurrentLayerIndex(layerIndex);
                } else {
                    mSettings.endGroup();
                    continue;
                }
            }
        }
        mSettings.endGroup();
    }
    mViewHint.valid = false;

    int lastActiveDocument =
            mSettings.value(QLatin1String("lastActive"), -1).toInt();
    if (lastActiveDocument >= 0 && lastActiveDocument < docman()->documentCount())
        ui->documentTabWidget->setCurrentIndex(lastActiveDocument);

    mSettings.endGroup();
}

#include "BuildingEditor/buildingtiles.h"
#include "BuildingEditor/buildingtemplates.h"
#include "BuildingEditor/buildingtmx.h"
#include "BuildingEditor/furnituregroups.h"
using namespace BuildingEditor;

// All this is needed for .tbx lots.
bool MainWindow::InitConfigFiles()
{
    // Refresh the ui before blocking while loading tilesets etc
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    if (!TileMetaInfoMgr::instance()->readTxt()) {
        QMessageBox::critical(this, tr("It's no good, Jim!"),
                              tr("%1\n(while reading %2)")
                              .arg(TileMetaInfoMgr::instance()->errorString())
                              .arg(TileMetaInfoMgr::instance()->txtName()));
        return false;
    }

    if (!BuildingTMX::instance()->readTxt()) {
        QMessageBox::critical(this, tr("It's no good, Jim!"),
                              tr("Error while reading %1\n%2")
                              .arg(BuildingTMX::instance()->txtName())
                              .arg(BuildingTMX::instance()->errorString()));
        return false;
    }

    if (!BuildingTilesMgr::instance()->readTxt()) {
        QMessageBox::critical(this, tr("It's no good, Jim!"),
                              tr("Error while reading %1\n%2")
                              .arg(BuildingTilesMgr::instance()->txtName())
                              .arg(BuildingTilesMgr::instance()->errorString()));
        return false;
    }

    if (!FurnitureGroups::instance()->readTxt()) {
        QMessageBox::critical(this, tr("It's no good, Jim!"),
                              tr("Error while reading %1\n%2")
                              .arg(FurnitureGroups::instance()->txtName())
                              .arg(FurnitureGroups::instance()->errorString()));
        return false;
    }

    if (!BuildingTemplates::instance()->readTxt()) {
        QMessageBox::critical(this, tr("It's no good, Jim!"),
                              tr("Error while reading %1\n%2")
                              .arg(BuildingTemplates::instance()->txtName())
                              .arg(BuildingTemplates::instance()->errorString()));
        return false;
    }

    return true;
}

void MainWindow::setStatusBarCoords(int x, int y)
{
    ui->coordinatesLabel->setText(QString(QLatin1String("%1,%2")).arg(x).arg(y));
}

void MainWindow::aboutToShowCurrentLevelMenu()
{
    mCurrentLevelMenu->clear();
    CellDocument *cellDoc = mCurrentDocument->asCellDocument();
    if (!cellDoc)
        return;
    QStringList items;
    foreach (CompositeLayerGroup *layerGroup, cellDoc->scene()->mapComposite()->sortedLayerGroups())
        items.prepend(QString::number(layerGroup->level()));
    foreach (QString item, items) {
        QAction *action = mCurrentLevelMenu->addAction(item);
        if (item.toInt() == cellDoc->currentLevel()) {
            action->setCheckable(true);
            action->setChecked(true);
            action->setEnabled(false);
        }
    }
}

void MainWindow::currentLevelMenuTriggered(QAction *action)
{
    CellDocument *cellDoc = mCurrentDocument->asCellDocument();
    if (!cellDoc)
        return;
    int level = action->text().toInt();
    cellDoc->setCurrentLevel(level);
}

void MainWindow::aboutToShowObjGrpMenu()
{
    mObjectGroupMenu->clear();
    CellDocument *cellDoc = mCurrentDocument->asCellDocument();
    if (!cellDoc)
        return;
    const ObjectGroupList &groups = cellDoc->world()->objectGroups();
    QAction *before = 0;
    foreach (WorldObjectGroup *og, groups) {
        QString name = og->name();
        if (name.isEmpty())
            name = tr("<none>");
        // This extra space is so the down arrow doesn't overlap the text
        name += QLatin1Char(' ');
        QAction *action = new QAction(name, mObjectGroupMenu);
        if (og == cellDoc->currentObjectGroup()) {
            action->setCheckable(true);
            action->setChecked(true);
            action->setEnabled(false);
        }
        mObjectGroupMenu->insertAction(before, action);
        before = action;
    }
}

void MainWindow::objGrpMenuTriggered(QAction *action)
{
    CellDocument *cellDoc = mCurrentDocument->asCellDocument();
    if (!cellDoc)
        return;
    int index = mObjectGroupMenu->actions().indexOf(action);
    const ObjectGroupList &groups = cellDoc->world()->objectGroups();
    index = groups.size() - index - 1;
    cellDoc->setCurrentObjectGroup(groups.at(index));
}

void MainWindow::lotpackviewer()
{
    if (!mLotPackWindow)
        mLotPackWindow = new LotPackWindow(this);

    mLotPackWindow->show();
    mLotPackWindow->activateWindow();
    mLotPackWindow->raise();
}

class FromToFile
{
public:
    bool read(const QString &fileName);

    QString errorString() const { return mError; }

    class FromTo
    {
    public:
        QStringList layers;
        QStringList from;
        QStringList to;
    };
    QList<FromTo> fromtos;

    QString mError;
};

bool FromToFile::read(const QString &fileName)
{
    SimpleFile simple;
    if (!simple.read(fileName)) {
        mError = simple.errorString();
        return false;
    }

    foreach (SimpleFileBlock b, simple.blocks) {
        SimpleFileKeyValue kv;
        if (b.name == QLatin1String("fromto")) {
            if (!b.hasValue("layers") || !b.hasValue("from") || !b.hasValue("to")) {
                mError = simple.tr("Line %1: Missing layers/from/to value.").arg(b.lineNumber);
                return false;
            }
            FromTo fromto;
            if (b.keyValue("layers", kv))
                fromto.layers = kv.values();
            if (b.keyValue("from", kv))
                fromto.from = kv.values();
            if (b.keyValue("to", kv))
                fromto.to = kv.values();
            foreach (QString tileName, fromto.from + fromto.to) {
                if (!BuildingTilesMgr::legalTileName(tileName)) {
                    mError = simple.tr("Invalid tile name '%1'").arg(tileName);
                    return false;
                }
            }

            fromtos += fromto;
        }
    }

    return true;
}

void MainWindow::FromToAll()
{
    FromToAux(false);
}

void MainWindow::FromToSelected()
{
    FromToAux(true);
}

#include "worldgen/worldgenwindow.h"
void MainWindow::WorldGen()
{
    WorldGen::WorldGenWindow::instance()->show();
    WorldGen::WorldGenWindow::instance()->raise();
    WorldGen::WorldGenWindow::instance()->activateWindow();
}

#include "mapwriter.h"
#include <QScopedPointer>
void MainWindow::FromToAux(bool selectedOnly)
{
    WorldDocument *worldDoc = mCurrentDocument->asWorldDocument();
    if (!worldDoc)
        return;
    FromToDialog dialog(worldDoc, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    QString f = dialog.rulesFile();
    if (f.isEmpty()) return;
    FromToFile file;
    if (!file.read(f)) {
        QMessageBox::warning(this, tr("Error reading from-to file"),
                             file.errorString());
        return;
    }

    if (file.fromtos.size() == 0) {
        QMessageBox::information(this, tr("Alias Fixup Error"),
                                 tr("That file has no fromto definitions!"));
        return;
    }

    QStringList fileNames;
    if (selectedOnly) {
        foreach (WorldCell *cell, worldDoc->selectedCells()) {
            f = cell->mapFilePath();
            if (f.isEmpty()) continue;
            if (fileNames.contains(f)) continue;
            fileNames += f;
        }
    } else {
        World *world = worldDoc->world();
        for (int y = 0; y < world->height(); y++) {
            for (int x = 0; x < world->width(); x++) {
                WorldCell *cell = world->cellAt(x, y);
                f = cell->mapFilePath();
                if (f.isEmpty()) continue;
                if (fileNames.contains(f)) continue;
                fileNames += f;
            }
        }
    }

    PROGRESS progress(tr("Making a mess of things"));

    foreach (QString fileName, fileNames) {
        if (MapInfo *mapInfo = MapManager::instance()->loadMap(fileName)) {
            QScopedPointer<Map> map(mapInfo->map()->clone());

            QMap<QString,TileLayer*> layerMapping;
            foreach (FromToFile::FromTo fromto, file.fromtos) {
                foreach (QString layerName, fromto.layers) {
                    int index = mapInfo->map()->indexOfLayer(layerName, Layer::TileLayerType);
                    if (index == -1) continue;
                    TileLayer *tl = map->layerAt(index)->asTileLayer();
                    layerMapping[layerName] = tl;
                }
            }

            QMap<QString,Tileset*> tilesetByName;
            foreach (Tileset *ts, map->tilesets()) {
                tilesetByName[ts->name()] = ts;
            }

            QMap<QPair<TileLayer*,Tile*>,QList<Tile*> > tileMapping;
            foreach (FromToFile::FromTo fromto, file.fromtos) {
                QString tilesetName;
                int tileID;

                QList<Tile*> fromTiles;
                foreach (QString tileName, fromto.from) {
                    BuildingTilesMgr::parseTileName(tileName, tilesetName, tileID);
                    if (tilesetByName.contains(tilesetName) && tilesetByName[tilesetName]->tileAt(tileID)) {
                        fromTiles += tilesetByName[tilesetName]->tileAt(tileID);
                    } else if (Tileset *ts = TileMetaInfoMgr::instance()->tileset(tilesetName)) {
                        TileMetaInfoMgr::instance()->loadTilesets(QList<Tileset*>() << ts);
                        if (Tile *tile = ts->tileAt(tileID)) {
                            map->addTileset(ts);
                            tilesetByName[tilesetName] = ts;
                            fromTiles += tile;
                        }
                    } else {
                        QString mError = tr("Map '%1' is missing tileset '%2' needed by the FromTo.txt file.\nThe tileset is not one of those in Tilesets.txt.")
                                .arg(QFileInfo(fileName).fileName()).arg(tilesetName);
                        QMessageBox::warning(this, tr("From/To Failed"), mError);
                        return;
                    }
                }
                QList<Tile*> toTiles;
                foreach (QString tileName, fromto.to) {
                    BuildingTilesMgr::parseTileName(tileName, tilesetName, tileID);
                    if (tilesetByName.contains(tilesetName) && tilesetByName[tilesetName]->tileAt(tileID)) {
                        toTiles += tilesetByName[tilesetName]->tileAt(tileID);
                    } else if (Tileset *ts = TileMetaInfoMgr::instance()->tileset(tilesetName)) {
                        TileMetaInfoMgr::instance()->loadTilesets(QList<Tileset*>() << ts);
                        if (Tile *tile = ts->tileAt(tileID)) {
                            map->addTileset(ts);
                            tilesetByName[tilesetName] = ts;
                            toTiles += tile;
                        }
                    } else {
                        QString mError = tr("Map '%1' is missing tileset '%2' needed by the FromTo.txt file.\nThe tileset is not one of those in Tilesets.txt.")
                                .arg(QFileInfo(fileName).fileName()).arg(tilesetName);
                        QMessageBox::warning(this, tr("From/To Failed"), mError);
                        return;
                    }
                }
                foreach (QString layerName, fromto.layers) {
                    if (!layerMapping.contains(layerName)) continue;
                    TileLayer *tl = layerMapping[layerName];
                    foreach (Tile *tile, fromTiles)
                        tileMapping[qMakePair(tl,tile)] = toTiles;
                }

            }

            foreach (TileLayer *tl, layerMapping.values()) {
                for (int x = 0; x < tl->width(); x++) {
                    for (int y = 0; y < tl->height(); y++) {
                        Cell cell = tl->cellAt(x, y);
                        if (!cell.tile) continue;
                        if (tileMapping.contains(qMakePair(tl,cell.tile))) {
                            QList<Tile*> &choices = tileMapping[qMakePair(tl,cell.tile)];
                            tl->setCell(x, y, Cell(choices[qrand() % choices.size()]));
                        }
                    }
                }
            }

            if (!dialog.backupDir().isEmpty()) {
            }

            if (!dialog.destDir().isEmpty()) {
            }

            MapWriter writer;
            MapWriter::LayerDataFormat format = MapWriter::CSV;
            if (worldDoc->world()->getBMPToTMXSettings().compress)
                format = MapWriter::Base64Zlib;
            writer.setLayerDataFormat(format);
            writer.setDtdEnabled(false);
            if (!writer.writeMap(map.data(), mapInfo->path())) {
                QMessageBox::warning(this, tr("Error writing TMX"), writer.errorString());
                return;
            }
        }
    }
}

void MainWindow::enableDeveloperFeatures()
{
    // TOP SECRET: PLEASE DON'T LET PEOPLE KNOW ABOUT THIS BECAUSE THE DEVS
    // DO NOT WANT MASSIVE SPOILERS FOR FANS OF THE GAME.
    QString sourcePath = QCoreApplication::applicationDirPath()
            + QLatin1Char('/') + QLatin1String("EnableDeveloperFeatures.txt");
    if (!QFileInfo(sourcePath).exists()) {
        ui->menuTools->menuAction()->setVisible(false);
//        ui->actionLotPackViewer->setVisible(false);
    }
}

bool MainWindow::saveFile()
{
    if (!mCurrentDocument)
        return false;

    const QString currentFileName = mCurrentDocument->fileName();

    if (currentFileName.endsWith(QLatin1String(".pzw"), Qt::CaseInsensitive))
        return saveFile(currentFileName);
    else
        return saveFileAs();
}

bool MainWindow::saveFileAs()
{
    QString suggestedFileName;
    if (mCurrentDocument && !mCurrentDocument->fileName().isEmpty()) {
        const QFileInfo fileInfo(mCurrentDocument->fileName());
        suggestedFileName = fileInfo.path();
        suggestedFileName += QLatin1Char('/');
        suggestedFileName += fileInfo.completeBaseName();
        suggestedFileName += QLatin1String(".pzw");
    } else {
        QString path = Preferences::instance()->openFileDirectory();
        if (path.isEmpty() || !QDir(path).exists())
            path = QDir::currentPath();
        suggestedFileName = path;
        suggestedFileName += QLatin1Char('/');
        suggestedFileName += tr("untitled.pzw");
    }

    const QString fileName =
            QFileDialog::getSaveFileName(this, QString(), suggestedFileName,
                                         tr("PZWorldEd world files (*.pzw)"));
    if (!fileName.isEmpty()) {
        Preferences::instance()->setOpenFileDirectory(QFileInfo(fileName).absolutePath());
        return saveFile(fileName);
    }
    return false;
}

void MainWindow::closeFile()
{
    if (confirmSave())
        docman()->closeCurrentDocument();
}

void MainWindow::closeAllFiles()
{
    if (confirmAllSave())
        docman()->closeAllDocuments();
}

void MainWindow::updateWindowTitle()
{
    QString fileName = mCurrentDocument ? mCurrentDocument->fileName() : QString();
    if (fileName.isEmpty())
        fileName = tr("Untitled");
    else {
        fileName = QDir::toNativeSeparators(fileName);
    }
    setWindowTitle(tr("[*]%1 - WorldEd").arg(fileName));
    setWindowFilePath(fileName);
    bool isModified = mCurrentDocument ? mCurrentDocument->isModified() : false;
    if (mCurrentDocument && mCurrentDocument->isCellDocument())
        isModified = mCurrentDocument->asCellDocument()->worldDocument()->isModified();
    setWindowModified(isModified);
}

static void generateLots(MainWindow *mainWin, Document *doc,
                         LotFilesManager::GenerateMode mode)
{
    if (!doc)
        return;
    WorldDocument *worldDoc = doc->asWorldDocument();
    if (!worldDoc)
        return;
    GenerateLotsDialog dialog(worldDoc, mainWin);
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (!LotFilesManager::instance()->generateWorld(worldDoc, mode)) {
        QMessageBox::warning(mainWin, mainWin->tr("Lot Generation Failed!"),
                             LotFilesManager::instance()->errorString());
    }
#if 0
    TileMetaInfoMgr::deleteInstance();
#endif
}

void MainWindow::generateLotsAll()
{
    generateLots(this, mCurrentDocument, LotFilesManager::GenerateAll);
}

void MainWindow::generateLotsSelected()
{
    generateLots(this, mCurrentDocument, LotFilesManager::GenerateSelected);
}

static void _BMPToTMX(MainWindow *mainWin, Document *doc,
                      BMPToTMX::GenerateMode mode)
{
    WorldDocument *worldDoc = doc->asWorldDocument();
    if (!worldDoc)
        return;
    BMPToTMXDialog dialog(worldDoc, mainWin);
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (!BMPToTMX::instance()->generateWorld(worldDoc, mode)) {
        QMessageBox::warning(mainWin, mainWin->tr("BMP To TMX Failed!"),
                             BMPToTMX::instance()->errorString());
    }
#if 0
    TileMetaInfoMgr::deleteInstance();
#endif
}

void MainWindow::BMPToTMXAll()
{
    _BMPToTMX(this, mCurrentDocument, BMPToTMX::GenerateAll);
}

void MainWindow::BMPToTMXSelected()
{
    _BMPToTMX(this, mCurrentDocument, BMPToTMX::GenerateSelected);
}

void MainWindow::resizeWorld()
{
    WorldDocument *worldDoc = mCurrentDocument->asWorldDocument();
    if (!worldDoc)
        return;
    ResizeWorldDialog dialog(worldDoc, this);
    dialog.exec();
}

void MainWindow::preferencesDialog()
{
    WorldDocument *worldDoc = 0;
    if (mCurrentDocument) {
        worldDoc = mCurrentDocument->asWorldDocument();
        if (CellDocument *cellDoc = mCurrentDocument->asCellDocument())
            worldDoc = cellDoc->worldDocument();
    }
    PreferencesDialog dialog(worldDoc, this);
    dialog.exec();
}

void MainWindow::objectGroupsDialog()
{
    if (!mCurrentDocument)
        return;
    WorldDocument *worldDoc = mCurrentDocument->asWorldDocument();
    if (CellDocument *cellDoc = mCurrentDocument->asCellDocument())
        worldDoc = cellDoc->worldDocument();
    ObjectGroupsDialog dialog(worldDoc, this);
    dialog.exec();
}

void MainWindow::objectTypesDialog()
{
    if (!mCurrentDocument)
        return;
    WorldDocument *worldDoc = mCurrentDocument->asWorldDocument();
    if (CellDocument *cellDoc = mCurrentDocument->asCellDocument())
        worldDoc = cellDoc->worldDocument();
    ObjectTypesDialog dialog(worldDoc, this);
    dialog.exec();
}

void MainWindow::properyDefinitionsDialog()
{
    if (!mCurrentDocument)
        return;
    WorldDocument *worldDoc = mCurrentDocument->asWorldDocument();
    if (CellDocument *cellDoc = mCurrentDocument->asCellDocument())
        worldDoc = cellDoc->worldDocument();
    PropertyDefinitionsDialog dialog(worldDoc, this);
    dialog.exec();
}

void MainWindow::templatesDialog()
{
    if (!mCurrentDocument)
        return;
    WorldDocument *worldDoc = mCurrentDocument->asWorldDocument();
    if (CellDocument *cellDoc = mCurrentDocument->asCellDocument())
        worldDoc = cellDoc->worldDocument();
    TemplatesDialog dialog(worldDoc, this);
    dialog.exec();
}

void MainWindow::copy()
{
    if (!mCurrentDocument)
        return;
    World *world = 0;
    if (WorldDocument *worldDoc = mCurrentDocument->asWorldDocument()) {
        CopyPasteDialog dialog(worldDoc, this);
        if (dialog.exec() == QDialog::Accepted)
            world = dialog.toWorld();
    }
    if (CellDocument *cellDoc = mCurrentDocument->asCellDocument()) {
        CopyPasteDialog dialog(cellDoc, this);
        if (dialog.exec() == QDialog::Accepted)
            world = dialog.toWorld();
    }
    if (world) {
        WorldWriter w;
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        w.writeWorld(world, &buffer, QDir::rootPath());
        qApp->clipboard()->setText(QString::fromUtf8(bytes.constData(),
                                                     bytes.size()));

        Clipboard::instance()->setWorld(world);
        updateActions();
    }
}

void MainWindow::paste()
{
    Q_ASSERT(mCurrentDocument && mCurrentDocument->isWorldDocument());
    WorldDocument *worldDoc = mCurrentDocument->asWorldDocument();
    if (Clipboard::instance()->cellsInClipboardCount())
        worldDoc->view()->scene()->asWorldScene()->pasteCellsFromClipboard();
    else
        Clipboard::instance()->pasteEverythingButCells(worldDoc);
}

void MainWindow::showClipboard()
{
    World *world = Clipboard::instance()->world();
    if (!world)
        return;
    CopyPasteDialog dialog(world, this);
    if (dialog.exec() == QDialog::Accepted) {
        world = dialog.toWorld();

        WorldWriter w;
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        w.writeWorld(world, &buffer, QDir::rootPath());
        qApp->clipboard()->setText(QString::fromUtf8(bytes.constData(),
                                                     bytes.size()));

        Clipboard::instance()->setWorld(world);
        updateActions();
    }
}

void MainWindow::removeRoad()
{
    Q_ASSERT(mCurrentDocument);
    WorldDocument *worldDoc = 0;
    if ((worldDoc = mCurrentDocument->asWorldDocument())) {
    }
    if (CellDocument *cellDoc = mCurrentDocument->asCellDocument()) {
        worldDoc = cellDoc->worldDocument();
    }
    int count = worldDoc->selectedRoadCount();
    Q_ASSERT(count);

    QUndoStack *undoStack = worldDoc->undoStack();
    undoStack->beginMacro(tr("Remove %1 Road%2").arg(count)
                          .arg((count > 1) ? QLatin1String("s") : QLatin1String("")));
    foreach (Road *road, worldDoc->selectedRoads()) {
        int index = worldDoc->world()->roads().indexOf(road);
        Q_ASSERT(index >= 0);
        worldDoc->removeRoad(index);
    }
    undoStack->endMacro();
}

void MainWindow::removeBMP()
{
    Q_ASSERT(mCurrentDocument);
    WorldDocument *worldDoc = mCurrentDocument->asWorldDocument();
    if (!worldDoc)
        return;
    int count = worldDoc->selectedBMPCount();
    Q_ASSERT(count);

    QUndoStack *undoStack = worldDoc->undoStack();
    undoStack->beginMacro(tr("Remove %1 BMP Image%2").arg(count)
                          .arg((count > 1) ? QLatin1String("s") : QLatin1String("")));
    foreach (WorldBMP *bmp, worldDoc->selectedBMPs())
        worldDoc->removeBMP(bmp);
    undoStack->endMacro();
}

void MainWindow::removeLot()
{
    Q_ASSERT(mCurrentDocument);
    WorldDocument *worldDoc = 0;
    WorldCell *cell = 0;
    QList<WorldCellLot*> lots;
    if ((worldDoc = mCurrentDocument->asWorldDocument())) {
        Q_ASSERT(worldDoc->selectedCellCount());
        cell = worldDoc->selectedCells().first();
        lots = worldDoc->selectedLots();
    }
    if (CellDocument *cellDoc = mCurrentDocument->asCellDocument()) {
        cell = cellDoc->cell();
        lots = cellDoc->selectedLots();
        worldDoc = cellDoc->worldDocument();
    }
    int count = lots.size();
    if (!worldDoc || !cell || !count) // none of these should ever be true
        return;
    QUndoStack *undoStack = worldDoc->undoStack();
    undoStack->beginMacro(tr("Remove %1 Lot%2").arg(count).arg((count > 1) ? QLatin1String("s") : QLatin1String("")));
    foreach (WorldCellLot *lot, lots) {
        int index = cell->lots().indexOf(lot);
        Q_ASSERT(index >= 0);
        worldDoc->removeCellLot(cell, index);
    }
    undoStack->endMacro();
}

void MainWindow::removeObject()
{
    Q_ASSERT(mCurrentDocument);
    WorldDocument *worldDoc = 0;
    WorldCell *cell = 0;
    QList<WorldCellObject*> objects;
    if ((worldDoc = mCurrentDocument->asWorldDocument())) {
        Q_ASSERT(worldDoc->selectedCellCount());
        cell = worldDoc->selectedCells().first();
        objects = worldDoc->selectedObjects();
    }
    if (CellDocument *cellDoc = mCurrentDocument->asCellDocument()) {
        cell = cellDoc->cell();
        objects = cellDoc->selectedObjects();
        worldDoc = cellDoc->worldDocument();
    }
    int count = objects.size();
    if (!worldDoc || !cell || !count) // none of these should ever be true
        return;
    QUndoStack *undoStack = worldDoc->undoStack();
    undoStack->beginMacro(tr("Remove %1 Object%2").arg(count).arg(count ? QLatin1String("s") : QLatin1String("")));
    foreach (WorldCellObject *obj, objects) {
        int index = cell->objects().indexOf(obj);
        Q_ASSERT(index >= 0);
        worldDoc->removeCellObject(cell, index);
    }
    undoStack->endMacro();
}

void MainWindow::extractLots()
{
    Q_ASSERT(mCurrentDocument);
    Q_ASSERT(mCurrentDocument->isCellDocument());
    CellDocument *cellDoc = mCurrentDocument->asCellDocument();
    QFileInfo info(cellDoc->scene()->mapComposite()->mapInfo()->path());
    QString message = tr("This command will create a new Lot for each 'lot' object " \
                         "that is in the cell's map \"%1\".  You should then " \
                         "remove those objects from the map using the TileZed " \
                         "editor, otherwise the Lots will be loaded twice by the game.")
            .arg(info.completeBaseName());
    QMessageBox::StandardButton b =
            QMessageBox::information(this, tr("Extract Lots"), message,
                                     QMessageBox::Ok, QMessageBox::Cancel);
    if (b == QMessageBox::Cancel)
        return;

    WorldDocument *worldDoc = cellDoc->worldDocument();
    worldDoc->undoStack()->beginMacro(tr("Extract Lots"));

    WorldCell *cell = cellDoc->cell();
    Map *map = cellDoc->scene()->map();
    foreach (ObjectGroup *og, map->objectGroups()) {
        foreach (MapObject *o, og->objects()) {
            if (o->name() == QLatin1String("lot") && !o->type().isEmpty()) {
                int x = qFloor(o->x()), y = qFloor(o->y());
                // Adjust for map orientation
                if (map->orientation() == Map::Isometric)
                    x += 3 * og->level(), y += 3 * og->level();
                WorldCellLot *lot = new WorldCellLot(cell,
                                                     o->type(), x, y,
                                                     og->level(),
                                                     o->width(), o->height());
               worldDoc->addCellLot(cell, cell->lots().size(), lot);
            }
        }
    }

    worldDoc->undoStack()->endMacro();
}

void MainWindow::extractObjects()
{
    Q_ASSERT(mCurrentDocument);
    Q_ASSERT(mCurrentDocument->isCellDocument());
    CellDocument *cellDoc = mCurrentDocument->asCellDocument();
    QFileInfo info(cellDoc->scene()->mapComposite()->mapInfo()->path());
    QString message = tr("This command will create a new Object for each object " \
                         "that is in the cell's map \"%1\" (except 'lot' objects).  "
                         "You should then remove those objects from the map using "
                         "the TileZed editor, otherwise the Objects will be loaded "
                         "twice by the game.")
            .arg(info.completeBaseName());
    QMessageBox::StandardButton b =
            QMessageBox::information(this, tr("Extract Objects"), message,
                                     QMessageBox::Ok, QMessageBox::Cancel);
    if (b == QMessageBox::Cancel)
        return;

    WorldDocument *worldDoc = cellDoc->worldDocument();
    worldDoc->undoStack()->beginMacro(tr("Extract Objects"));

    World *world = worldDoc->world();
    WorldCell *cell = cellDoc->cell();
    Map *map = cellDoc->scene()->map();
    foreach (ObjectGroup *og, map->objectGroups()) {
        foreach (MapObject *o, og->objects()) {
            // Note: object name/type reversed in TileZed
            if (o->name() != QLatin1String("lot") && !o->name().isEmpty()) {
                // Create a new ObjectGroup if needed
                WorldObjectGroup *group = world->nullObjectGroup();
                QString name = MapComposite::layerNameWithoutPrefix(og->name());
                if (!name.isEmpty()) {
                    group = world->objectGroups().find(name);
                    if (!group) {
                        group = new WorldObjectGroup(world, name);
                        worldDoc->addObjectGroup(group);
                    }
                }
                // Create a new ObjectType if needed
                ObjectType *type = world->objectTypes().find(o->name());
                if (!type) {
                    type = new ObjectType(o->name());
                    worldDoc->addObjectType(type);
                }
                // Adjust coordinates to whole numbers
                // I'm trying to match what PZ's Lot Creator does.
                int x = qFloor(o->x()), y = qFloor(o->y());
                int x2 = qCeil(o->x() + o->width()), y2 = qCeil(o->y() + o->height());
                int width = x2 - x, height = y2 - y;
                width = qMax(MIN_OBJECT_SIZE, qreal(width));
                height = qMax(MIN_OBJECT_SIZE, qreal(height));

                // Adjust for map orientation
                if (map->orientation() == Map::Isometric)
                    x += 3 * og->level(), y += 3 * og->level();

                WorldCellObject *obj = new WorldCellObject(cell, o->type(),
                                                           type, group, x, y,
                                                           og->level(),
                                                           width, height);
                worldDoc->addCellObject(cell, cell->objects().size(), obj);
            }
        }
    }

    worldDoc->undoStack()->endMacro();
}

void MainWindow::clearCells()
{
    Q_ASSERT(mCurrentDocument);
    WorldDocument *worldDoc = 0;
    QList<WorldCell*> cells;
    if ((worldDoc = mCurrentDocument->asWorldDocument())) {
        Q_ASSERT(worldDoc->selectedCellCount());
        cells = worldDoc->selectedCells();
    }
    if (CellDocument *cellDoc = mCurrentDocument->asCellDocument()) {
        cells += cellDoc->cell();
        worldDoc = cellDoc->worldDocument();
    }
    int count = cells.size();
    if (!worldDoc || !count) // none of these should ever be true
        return;
    QUndoStack *undoStack = worldDoc->undoStack();
    undoStack->beginMacro(tr("Clear %1 Cell%2").arg(count).arg((count > 1) ? QLatin1String("s") : QLatin1String("")));
    foreach (WorldCell *cell, cells)
        worldDoc->clearCell(cell);
    undoStack->endMacro();
}

void MainWindow::clearMapOnly()
{
    Q_ASSERT(mCurrentDocument);
    WorldDocument *worldDoc = 0;
    QList<WorldCell*> cells;
    if ((worldDoc = mCurrentDocument->asWorldDocument())) {
        Q_ASSERT(worldDoc->selectedCellCount());
        cells = worldDoc->selectedCells();
    }
    if (CellDocument *cellDoc = mCurrentDocument->asCellDocument()) {
        cells += cellDoc->cell();
        worldDoc = cellDoc->worldDocument();
    }
    int count = cells.size();
    if (!worldDoc || !count) // none of these should ever be true
        return;
    QUndoStack *undoStack = worldDoc->undoStack();
    undoStack->beginMacro(tr("Clear %1 Cell%2 Map").arg(count).arg((count > 1) ? QLatin1String("s") : QLatin1String("")));
    foreach (WorldCell *cell, cells)
        worldDoc->setCellMapName(cell, QString());
    undoStack->endMacro();
}

bool MainWindow::confirmSave()
{
    if (!mCurrentDocument || !mCurrentDocument->isModified())
        return true;

    int ret = QMessageBox::warning(
            this, tr("Unsaved Changes"),
            tr("There are unsaved changes. Do you want to save now?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    switch (ret) {
    case QMessageBox::Save:    return saveFile();
    case QMessageBox::Discard: return true;
    case QMessageBox::Cancel:
    default:
        return false;
    }
}

bool MainWindow::confirmAllSave()
{
    foreach (Document *doc, docman()->documents()) {
        if (!doc->isModified())
            continue;
        docman()->setCurrentDocument(doc);
        if (!confirmSave())
            return false;
    }

    return true;
}

void MainWindow::writeSettings()
{
    mSettings.beginGroup(QLatin1String("mainwindow"));
    mSettings.setValue(QLatin1String("geometry"), saveGeometry());
    mSettings.setValue(QLatin1String("state"), saveState());
    mSettings.endGroup();

    mSettings.beginGroup(QLatin1String("openFiles"));
    mSettings.remove(QLatin1String(""));
    if (mCurrentDocument)
        mSettings.setValue(QLatin1String("lastActive"),
                           docman()->indexOf(mCurrentDocument));

    mSettings.setValue(QLatin1String("count"), docman()->documentCount());

    int i = 0;
    foreach (Document *doc, docman()->documents()) {
        mSettings.beginGroup(QString::number(i)); // openFiles/N/...

        mSettings.setValue(QLatin1String("file"), doc->fileName());
        BaseGraphicsView *view = doc->view();

        mSettings.setValue(QLatin1String("scale"),
                           QString::number(view->zoomable()->scale()));

        QPointF centerScenePos = view->mapToScene(view->width() / 2, view->height() / 2);
        mSettings.setValue(QLatin1String("scrollX"),
                           QString::number(int(centerScenePos.x())));
        mSettings.setValue(QLatin1String("scrollY"),
                           QString::number(int(centerScenePos.y())));

        if (CellDocument *cellDoc = doc->asCellDocument()) {
            mSettings.setValue(QLatin1String("cellX"), QString::number(cellDoc->cell()->x()));
            mSettings.setValue(QLatin1String("cellY"), QString::number(cellDoc->cell()->y()));
            const int currentLayerIndex = cellDoc->currentLayerIndex();
            mSettings.setValue(QLatin1String("currentLayer"), QString::number(currentLayerIndex));
        } else {
        }
        mSettings.endGroup();
        ++i;
    }

    mSettings.endGroup();
}

void MainWindow::readSettings()
{
    mSettings.beginGroup(QLatin1String("mainwindow"));
    QByteArray geom = mSettings.value(QLatin1String("geometry")).toByteArray();
    if (!geom.isEmpty())
        restoreGeometry(geom);
    else
        resize(800, 600);
    restoreState(mSettings.value(QLatin1String("state"),
                                 QByteArray()).toByteArray());
    mSettings.endGroup();
//    updateRecentFiles();
}

bool MainWindow::saveFile(const QString &fileName)
{
    if (!mCurrentDocument)
        return false;

    QString error;
    if (!mCurrentDocument->save(fileName, error)) {
        QMessageBox::critical(this, tr("Error Saving Map"), error);
        return false;
    }

    updateWindowTitle();

    // Update tab tooltips
    WorldDocument *worldDoc = mCurrentDocument->asWorldDocument();
    if (!worldDoc)
        worldDoc = mCurrentDocument->asCellDocument()->worldDocument();
    int pos = 0;
    foreach (Document *doc, docman()->documents()) {
        CellDocument *cellDoc = doc->asCellDocument();
        if ((doc == worldDoc) ||
                (cellDoc && cellDoc->worldDocument() == worldDoc)) {
            ui->documentTabWidget->setTabToolTip(pos, fileName);
        }
        ++pos;
    }

//    setRecentFile(fileName);
    return true;
}

void MainWindow::updateActions()
{
    Document *doc = mCurrentDocument;
    bool hasDoc = doc != 0;
    CellDocument *cellDoc = hasDoc ? doc->asCellDocument() : 0;
    WorldDocument *worldDoc = hasDoc ? doc->asWorldDocument() : 0;

    ui->actionSave->setEnabled(hasDoc);
    ui->actionSaveAs->setEnabled(hasDoc);
    ui->actionClose->setEnabled(hasDoc);
    ui->actionCloseAll->setEnabled(hasDoc);

    ui->actionGenerateLotsAll->setEnabled(worldDoc);
    ui->actionGenerateLotsSelected->setEnabled(worldDoc &&
                                               worldDoc->selectedCellCount());

    ui->actionBMPToTMXAll->setEnabled(worldDoc);
    ui->actionBMPToTMXSelected->setEnabled(worldDoc &&
                                           worldDoc->selectedCellCount());

    ui->actionCopy->setEnabled(worldDoc);
    ui->actionPaste->setEnabled(worldDoc && !Clipboard::instance()->isEmpty());

    bool removeRoad = (worldDoc && worldDoc->selectedRoadCount()) ||
            (cellDoc && cellDoc->worldDocument()->selectedRoadCount());
    ui->actionRemoveRoad->setEnabled(removeRoad);

    ui->actionRemoveBMP->setEnabled(worldDoc && worldDoc->selectedBMPCount());

    ui->actionEditCell->setEnabled(false);
    ui->actionResizeWorld->setEnabled(worldDoc);
    ui->actionObjectTypes->setEnabled(hasDoc);
    ui->actionProperties->setEnabled(hasDoc);
    ui->actionTemplates->setEnabled(hasDoc);

    bool removeLot = (cellDoc && cellDoc->selectedLotCount())
            || (worldDoc && worldDoc->selectedLotCount());
    ui->actionRemoveLot->setEnabled(removeLot);

    bool removeObject = (cellDoc && cellDoc->selectedObjectCount())
            || (worldDoc && worldDoc->selectedObjectCount());
    ui->actionRemoveObject->setEnabled(removeObject);

    ui->actionExtractLots->setEnabled(cellDoc != 0);
    ui->actionExtractObjects->setEnabled(cellDoc != 0);
    ui->actionClearCell->setEnabled(false);
    ui->actionClearMapOnly->setEnabled(false);

    ui->actionSnapToGrid->setEnabled(cellDoc != 0);
    ui->actionShowCoordinates->setEnabled(worldDoc != 0);

    Preferences *prefs = Preferences::instance();
    ui->actionShowGrid->setChecked(worldDoc ? prefs->showWorldGrid() : cellDoc ? prefs->showCellGrid() : false);
    ui->actionShowGrid->setEnabled(hasDoc);

    ui->actionHighlightCurrentLevel->setEnabled(cellDoc != 0);

    ui->actionLevelAbove->setEnabled(false);
    ui->actionLevelBelow->setEnabled(false);

    updateZoom();

    if (worldDoc) {
        WorldCell *cell = worldDoc->selectedCellCount() ? worldDoc->selectedCells().first() : 0;
        if (cell) {
            ui->actionEditCell->setEnabled(true);
            ui->actionClearCell->setEnabled(true);
            ui->actionClearMapOnly->setEnabled(true);
            ui->currentCellLabel->setText(tr("Current cell: %1,%2").arg(cell->x()).arg(cell->y()));
        } else
            ui->currentCellLabel->setText(tr("Current cell: <none>"));
        ui->currentLevelButton->setText(tr("Level: ? ")); // extra space cuz of down-arrow placement on Windows
        ui->currentLevelButton->setEnabled(false);
        ui->objectGroupButton->setText(tr("Obj Grp: <none> "));
        ui->objectGroupButton->setEnabled(false);
    } else if (cellDoc) {
        ui->actionClearCell->setEnabled(true);
        ui->actionClearMapOnly->setEnabled(true);
        WorldCell *cell = cellDoc->cell();
        ui->currentCellLabel->setText(tr("Current cell: %1,%2").arg(cell->x()).arg(cell->y()));
        int level = cellDoc->currentLevel();
        ui->currentLevelButton->setText(tr("Level: %1 ").arg(level)); // extra space cuz of down-arrow placement on Windows
        ui->currentLevelButton->setEnabled(true);
        ui->actionLevelAbove->setEnabled(level < cellDoc->scene()->mapComposite()->maxLevel());
        ui->actionLevelBelow->setEnabled(level > 0);
        WorldObjectGroup *og = cellDoc->currentObjectGroup();
        ui->objectGroupButton->setText(tr("Obj Grp: %1 ")
                                       .arg((og && !og->name().isEmpty())
                                       ? og->name() : tr("<none>")));
        ui->objectGroupButton->setEnabled(true);
    } else {
        ui->coordinatesLabel->clear();
        ui->currentCellLabel->setText(tr("Current cell: <none> "));
        ui->currentLevelButton->setText(tr("Level: ? ")); // extra space cuz of down-arrow placement on Windows
        ui->currentLevelButton->setEnabled(false);
        ui->objectGroupButton->setText(tr("Obj Grp: <none> "));
        ui->objectGroupButton->setEnabled(false);
    }
}

void MainWindow::updateZoom()
{
    const qreal scale = mZoomable ? mZoomable->scale() : 1;

    ui->actionZoomIn->setEnabled(mZoomable && mZoomable->canZoomIn());
    ui->actionZoomOut->setEnabled(mZoomable && mZoomable->canZoomOut());
    ui->actionZoomNormal->setEnabled(scale != 1);

    if (mZoomable) {
        mZoomComboBox->setEnabled(true);
    } else {
        int index = mZoomComboBox->findData((qreal)1.0);
        mZoomComboBox->setCurrentIndex(index);
        mZoomComboBox->setEnabled(false);
    }
}

#include "luaconsole.h"
void MainWindow::LuaConsole()
{
    LuaConsole::instance()->show();
    LuaConsole::instance()->raise();
    LuaConsole::instance()->activateWindow();
}

void MainWindow::switchOrthoIso()
{
    if (PathDocument *doc = mCurrentDocument->asPathDocument()) {
        if (doc->view()->scene()->isIso())
            doc->view()->switchToOrtho();
        else if (doc->view()->scene()->isOrtho())
            doc->view()->switchToTile();
        else
            doc->view()->switchToIso();
    }
}
