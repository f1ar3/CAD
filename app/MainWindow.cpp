#include "MainWindow.h"
#include "../src/viewer/OccView.h"
#include "../src/model/Document.h"
#include "../src/io/FileIO.h"
#include "../src/widgets/PrimitiveDialog.h"
#include "../src/widgets/TransformDialog.h"
#include "../src/widgets/BooleanDialog.h"
#include "../src/widgets/ModelTreeWidget.h"
#include "../src/widgets/PropertyPanel.h"
#include "../src/widgets/MeasureDialog.h"
#include "../src/widgets/PatternDialog.h"
#include "../src/widgets/MirrorDialog.h"
#include "../src/widgets/SketchDialog.h"
#include "../src/widgets/ExtrudeDialog.h"

#include <QMenuBar>
#include <QToolBar>
#include <QToolButton>
#include <QStatusBar>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>
#include <QStyle>
#include <QFileInfo>
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Мини-САПР"));
    resize(1280, 800);

    m_occView = new OccView(this);
    setCentralWidget(m_occView);

    createActions();
    createMenus();
    createToolBars();
    createDockWidgets();

    statusBar()->showMessage(tr("Готово. Создавайте фигуры через меню Моделирование или панель инструментов."));

    QMetaObject::invokeMethod(this, &MainWindow::initDocument, Qt::QueuedConnection);
}

MainWindow::~MainWindow() = default;

void MainWindow::initDocument()
{
    if (m_occView->context().IsNull()) {
        m_occView->repaint();
    }
    m_document = new Document(m_occView->context(), this);

    m_modelTree->setDocument(m_document);
    m_propertyPanel->setDocument(m_document);

    connect(m_document, &Document::selectionChanged, this, &MainWindow::onSelectionChanged);
    connect(m_occView, &OccView::selectionChanged, this, &MainWindow::onViewerSelectionChanged);

    // Drag signals
    connect(m_occView, &OccView::dragStarted, this, [this](int /*viewId*/) {
        if (!m_document) return;
        m_draggedShapeId = m_document->selectedShapeId();
        if (m_draggedShapeId >= 0) m_document->beginDrag(m_draggedShapeId);
    });
    connect(m_occView, &OccView::dragMoved, this, [this](int /*viewId*/, double dx, double dy, double dz) {
        if (!m_document || m_draggedShapeId < 0) return;
        m_document->dragShape(m_draggedShapeId, dx, dy, dz);
    });
    connect(m_occView, &OccView::dragFinished, this, [this](int /*viewId*/) {
        if (!m_document || m_draggedShapeId < 0) return;
        m_document->finishDrag(m_draggedShapeId);
        m_propertyPanel->showShape(m_draggedShapeId);
        statusBar()->showMessage(tr("Фигура перемещена"));
        m_draggedShapeId = -1;
    });

    // Undo / Redo
    connect(m_actUndo, &QAction::triggered, this, [this]() {
        if (!m_document) return;
        m_document->undo();
        m_occView->fitAll();
        statusBar()->showMessage(tr("Отменено"));
    });
    connect(m_actRedo, &QAction::triggered, this, [this]() {
        if (!m_document) return;
        m_document->redo();
        m_occView->fitAll();
        statusBar()->showMessage(tr("Повторено"));
    });
    connect(m_document, &Document::undoRedoChanged, this, [this]() {
        m_actUndo->setEnabled(m_document->canUndo());
        m_actRedo->setEnabled(m_document->canRedo());

        QString undoTip = m_document->canUndo()
            ? tr("Отменить: %1").arg(m_document->undoDescription())
            : tr("Отменить");
        QString redoTip = m_document->canRedo()
            ? tr("Повторить: %1").arg(m_document->redoDescription())
            : tr("Повторить");
        m_actUndo->setToolTip(undoTip);
        m_actRedo->setToolTip(redoTip);
    });
}

// ============================================================
//  Действия (Actions)
// ============================================================

void MainWindow::createActions()
{
    auto icon = [](const QString& name) {
        return QIcon(QString(":/icons/%1").arg(name));
    };

    // --- Файл ---
    m_actNew = new QAction(icon("new_doc"), tr("Новый"), this);
    m_actNew->setShortcut(QKeySequence::New);
    m_actNew->setToolTip(tr("Создать новый пустой документ (Cmd+N)"));
    connect(m_actNew, &QAction::triggered, this, &MainWindow::onNewDocument);

    m_actOpen = new QAction(icon("open"), tr("Открыть..."), this);
    m_actOpen->setShortcut(QKeySequence::Open);
    m_actOpen->setToolTip(tr("Импортировать файл STEP/IGES/BRep (Cmd+O)"));
    connect(m_actOpen, &QAction::triggered, this, &MainWindow::onOpenFile);

    m_actSaveAs = new QAction(icon("save"), tr("Сохранить как..."), this);
    m_actSaveAs->setShortcut(QKeySequence::SaveAs);
    m_actSaveAs->setToolTip(tr("Экспортировать в STEP/IGES/BRep (Cmd+Shift+S)"));
    connect(m_actSaveAs, &QAction::triggered, this, &MainWindow::onSaveAs);

    m_actExit = new QAction(tr("Выход"), this);
    m_actExit->setShortcut(QKeySequence::Quit);
    connect(m_actExit, &QAction::triggered, this, &QWidget::close);

    // --- Правка ---
    m_actUndo = new QAction(icon("undo"), tr("Отменить"), this);
    m_actUndo->setShortcut(QKeySequence::Undo);
    m_actUndo->setToolTip(tr("Отменить (Cmd+Z)"));
    m_actUndo->setEnabled(false);

    m_actRedo = new QAction(icon("redo"), tr("Повторить"), this);
    m_actRedo->setShortcut(QKeySequence::Redo);
    m_actRedo->setToolTip(tr("Повторить (Cmd+Shift+Z)"));
    m_actRedo->setEnabled(false);

    m_actDelete = new QAction(icon("delete"), tr("Удалить"), this);
    m_actDelete->setShortcut(QKeySequence::Delete);
    m_actDelete->setToolTip(tr("Удалить выделенную фигуру (Del)"));
    connect(m_actDelete, &QAction::triggered, this, &MainWindow::onDeleteSelected);

    // --- Вид ---
    m_actFitAll = new QAction(icon("fitall"), tr("Показать всё"), this);
    m_actFitAll->setShortcut(tr("F"));
    m_actFitAll->setToolTip(tr("Вписать все фигуры в окно (F)"));
    connect(m_actFitAll, &QAction::triggered, this, &MainWindow::onViewFitAll);

    m_actViewFront = new QAction(icon("view_front"), tr("Спереди"), this);
    m_actViewFront->setShortcut(tr("1"));
    m_actViewFront->setToolTip(tr("Вид спереди (1)"));
    connect(m_actViewFront, &QAction::triggered, this, &MainWindow::onViewFront);

    m_actViewTop = new QAction(icon("view_top"), tr("Сверху"), this);
    m_actViewTop->setShortcut(tr("2"));
    m_actViewTop->setToolTip(tr("Вид сверху (2)"));
    connect(m_actViewTop, &QAction::triggered, this, &MainWindow::onViewTop);

    m_actViewRight = new QAction(icon("view_right"), tr("Справа"), this);
    m_actViewRight->setShortcut(tr("3"));
    m_actViewRight->setToolTip(tr("Вид справа (3)"));
    connect(m_actViewRight, &QAction::triggered, this, &MainWindow::onViewRight);

    m_actViewIso = new QAction(icon("view_iso"), tr("Изометрия"), this);
    m_actViewIso->setShortcut(tr("0"));
    m_actViewIso->setToolTip(tr("Изометрический вид (0)"));
    connect(m_actViewIso, &QAction::triggered, this, &MainWindow::onViewIso);

    // --- Примитивы ---
    m_actBox = new QAction(icon("box"), tr("Параллелепипед"), this);
    m_actBox->setToolTip(tr("Создать параллелепипед"));
    connect(m_actBox, &QAction::triggered, this, &MainWindow::onCreateBox);

    m_actCylinder = new QAction(icon("cylinder"), tr("Цилиндр"), this);
    m_actCylinder->setToolTip(tr("Создать цилиндр"));
    connect(m_actCylinder, &QAction::triggered, this, &MainWindow::onCreateCylinder);

    m_actSphere = new QAction(icon("sphere"), tr("Сфера"), this);
    m_actSphere->setToolTip(tr("Создать сферу"));
    connect(m_actSphere, &QAction::triggered, this, &MainWindow::onCreateSphere);

    m_actCone = new QAction(icon("cone"), tr("Конус"), this);
    m_actCone->setToolTip(tr("Создать конус"));
    connect(m_actCone, &QAction::triggered, this, &MainWindow::onCreateCone);

    m_actTorus = new QAction(icon("torus"), tr("Тор"), this);
    m_actTorus->setToolTip(tr("Создать тор"));
    connect(m_actTorus, &QAction::triggered, this, &MainWindow::onCreateTorus);

    m_actWedge = new QAction(icon("wedge"), tr("Клин"), this);
    m_actWedge->setToolTip(tr("Создать клин"));
    connect(m_actWedge, &QAction::triggered, this, &MainWindow::onCreateWedge);

    // --- Булевы операции ---
    m_actFuse = new QAction(icon("bool_fuse"), tr("Объединение"), this);
    m_actFuse->setToolTip(tr("Объединить две фигуры в одну"));
    connect(m_actFuse, &QAction::triggered, this, &MainWindow::onBoolFuse);

    m_actCut = new QAction(icon("bool_cut"), tr("Вычитание"), this);
    m_actCut->setToolTip(tr("Вычесть одну фигуру из другой"));
    connect(m_actCut, &QAction::triggered, this, &MainWindow::onBoolCut);

    m_actCommon = new QAction(icon("bool_common"), tr("Пересечение"), this);
    m_actCommon->setToolTip(tr("Оставить только пересечение двух фигур"));
    connect(m_actCommon, &QAction::triggered, this, &MainWindow::onBoolCommon);

    // --- Скругление / Фаска ---
    m_actFillet = new QAction(icon("fillet"), tr("Скругление..."), this);
    m_actFillet->setToolTip(tr("Скруглить все рёбра выделенной фигуры"));
    connect(m_actFillet, &QAction::triggered, this, &MainWindow::onFillet);

    m_actChamfer = new QAction(icon("chamfer"), tr("Фаска..."), this);
    m_actChamfer->setToolTip(tr("Снять фаску со всех рёбер выделенной фигуры"));
    connect(m_actChamfer, &QAction::triggered, this, &MainWindow::onChamfer);

    // --- Трансформации ---
    m_actTranslate = new QAction(icon("translate"), tr("Переместить..."), this);
    m_actTranslate->setShortcut(tr("G"));
    m_actTranslate->setToolTip(tr("Переместить фигуру (G)"));
    connect(m_actTranslate, &QAction::triggered, this, &MainWindow::onTranslate);

    m_actRotate = new QAction(icon("rotate"), tr("Повернуть..."), this);
    m_actRotate->setShortcut(tr("R"));
    m_actRotate->setToolTip(tr("Повернуть фигуру (R)"));
    connect(m_actRotate, &QAction::triggered, this, &MainWindow::onRotate);

    m_actScale = new QAction(icon("scale"), tr("Масштабировать..."), this);
    m_actScale->setShortcut(tr("S"));
    m_actScale->setToolTip(tr("Масштабировать фигуру (S)"));
    connect(m_actScale, &QAction::triggered, this, &MainWindow::onScale);

    // --- Измерение / Массив / Зеркало ---
    m_actMeasure = new QAction(icon("measure"), tr("Измерения..."), this);
    m_actMeasure->setShortcut(tr("M"));
    m_actMeasure->setToolTip(tr("Объём, площадь и габариты фигуры (M)"));
    connect(m_actMeasure, &QAction::triggered, this, &MainWindow::onMeasure);

    m_actPattern = new QAction(icon("pattern"), tr("Массив..."), this);
    m_actPattern->setShortcut(tr("P"));
    m_actPattern->setToolTip(tr("Линейный или круговой массив копий (P)"));
    connect(m_actPattern, &QAction::triggered, this, &MainWindow::onPattern);

    m_actMirror = new QAction(icon("mirror"), tr("Зеркало..."), this);
    m_actMirror->setToolTip(tr("Зеркальное отражение фигуры"));
    connect(m_actMirror, &QAction::triggered, this, &MainWindow::onMirror);

    m_actDuplicate = new QAction(icon("duplicate"), tr("Дублировать"), this);
    m_actDuplicate->setShortcut(QKeySequence(tr("Ctrl+D")));
    m_actDuplicate->setToolTip(tr("Дублировать выделенную фигуру (Ctrl+D)"));
    connect(m_actDuplicate, &QAction::triggered, this, &MainWindow::onDuplicate);

    m_actToggleVisibility = new QAction(icon("visibility"), tr("Скрыть/Показать"), this);
    m_actToggleVisibility->setShortcut(tr("H"));
    m_actToggleVisibility->setToolTip(tr("Скрыть или показать выделенную фигуру (H)"));
    connect(m_actToggleVisibility, &QAction::triggered, this, &MainWindow::onToggleVisibility);

    // --- 2D-эскизы ---
    m_actSketchRect = new QAction(icon("sketch_rect"), tr("Прямоугольник"), this);
    m_actSketchRect->setToolTip(tr("Эскиз: прямоугольник"));
    connect(m_actSketchRect, &QAction::triggered, this, &MainWindow::onSketchRectangle);

    m_actSketchCircle = new QAction(icon("sketch_circle"), tr("Окружность"), this);
    m_actSketchCircle->setToolTip(tr("Эскиз: окружность"));
    connect(m_actSketchCircle, &QAction::triggered, this, &MainWindow::onSketchCircle);

    m_actSketchEllipse = new QAction(icon("sketch_ellipse"), tr("Эллипс"), this);
    m_actSketchEllipse->setToolTip(tr("Эскиз: эллипс"));
    connect(m_actSketchEllipse, &QAction::triggered, this, &MainWindow::onSketchEllipse);

    m_actSketchPolygon = new QAction(icon("sketch_polygon"), tr("Многоугольник"), this);
    m_actSketchPolygon->setToolTip(tr("Эскиз: правильный многоугольник"));
    connect(m_actSketchPolygon, &QAction::triggered, this, &MainWindow::onSketchPolygon);

    m_actSketchTriangle = new QAction(icon("sketch_triangle"), tr("Треугольник"), this);
    m_actSketchTriangle->setToolTip(tr("Эскиз: равносторонний треугольник"));
    connect(m_actSketchTriangle, &QAction::triggered, this, &MainWindow::onSketchTriangle);

    // --- Выдавливание ---
    m_actExtrude = new QAction(icon("extrude"), tr("Выдавить..."), this);
    m_actExtrude->setShortcut(tr("E"));
    m_actExtrude->setToolTip(tr("Выдавить 2D-эскиз в 3D-тело (E)"));
    connect(m_actExtrude, &QAction::triggered, this, &MainWindow::onExtrude);
}

// ============================================================
//  Меню
// ============================================================

void MainWindow::createMenus()
{
    // Файл
    QMenu* fileMenu = menuBar()->addMenu(tr("&Файл"));
    fileMenu->addAction(m_actNew);
    fileMenu->addAction(m_actOpen);
    fileMenu->addAction(m_actSaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actExit);

    // Правка
    QMenu* editMenu = menuBar()->addMenu(tr("&Правка"));
    editMenu->addAction(m_actUndo);
    editMenu->addAction(m_actRedo);
    editMenu->addSeparator();
    editMenu->addAction(m_actDuplicate);
    editMenu->addAction(m_actDelete);

    // Вид
    QMenu* viewMenu = menuBar()->addMenu(tr("&Вид"));
    viewMenu->addAction(m_actFitAll);
    viewMenu->addSeparator();
    viewMenu->addAction(m_actViewFront);
    viewMenu->addAction(m_actViewTop);
    viewMenu->addAction(m_actViewRight);
    viewMenu->addAction(m_actViewIso);
    viewMenu->addSeparator();
    viewMenu->addAction(m_actToggleVisibility);

    // Эскиз
    QMenu* sketchMenu = menuBar()->addMenu(tr("&Эскиз"));
    sketchMenu->addAction(m_actSketchRect);
    sketchMenu->addAction(m_actSketchCircle);
    sketchMenu->addAction(m_actSketchEllipse);
    sketchMenu->addAction(m_actSketchPolygon);
    sketchMenu->addAction(m_actSketchTriangle);
    sketchMenu->addSeparator();
    sketchMenu->addAction(m_actExtrude);

    // Моделирование
    QMenu* modelMenu = menuBar()->addMenu(tr("&Моделирование"));
    modelMenu->addAction(m_actBox);
    modelMenu->addAction(m_actCylinder);
    modelMenu->addAction(m_actSphere);
    modelMenu->addAction(m_actCone);
    modelMenu->addAction(m_actTorus);
    modelMenu->addAction(m_actWedge);
    modelMenu->addSeparator();
    modelMenu->addAction(m_actFuse);
    modelMenu->addAction(m_actCut);
    modelMenu->addAction(m_actCommon);
    modelMenu->addSeparator();
    modelMenu->addAction(m_actFillet);
    modelMenu->addAction(m_actChamfer);
    modelMenu->addSeparator();
    modelMenu->addAction(m_actMeasure);
    modelMenu->addAction(m_actPattern);
    modelMenu->addAction(m_actMirror);

    // Трансформация
    QMenu* transformMenu = menuBar()->addMenu(tr("&Трансформация"));
    transformMenu->addAction(m_actTranslate);
    transformMenu->addAction(m_actRotate);
    transformMenu->addAction(m_actScale);
}

// ============================================================
//  Панели инструментов
// ============================================================

void MainWindow::createToolBars()
{
    const int iconSz = 22;

    // Вспомогательная: вертикальный разделитель между группами
    auto addGroupSep = [](QToolBar* bar) {
        auto* sep = new QFrame(bar);
        sep->setFrameShape(QFrame::VLine);
        sep->setFrameShadow(QFrame::Sunken);
        sep->setFixedWidth(2);
        sep->setFixedHeight(28);
        sep->setStyleSheet("QFrame { color: rgba(255,255,255,40); margin: 0 4px; }");
        bar->addWidget(sep);
    };

    // Вспомогательная: подпись группы
    auto addGroupLabel = [](QToolBar* bar, const QString& text) {
        auto* label = new QLabel(text, bar);
        label->setStyleSheet(
            "QLabel { color: rgba(255,255,255,120); font-size: 9px; padding: 0 3px; }");
        bar->addWidget(label);
    };

    // ========== Главная панель ==========
    QToolBar* mainBar = addToolBar(tr("Главная"));
    mainBar->setIconSize(QSize(iconSz, iconSz));
    mainBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    mainBar->setMovable(false);

    // --- Файл ---
    addGroupLabel(mainBar, tr("Файл"));
    mainBar->addAction(m_actNew);
    mainBar->addAction(m_actOpen);
    mainBar->addAction(m_actSaveAs);

    addGroupSep(mainBar);

    // --- Undo / Redo ---
    mainBar->addAction(m_actUndo);
    mainBar->addAction(m_actRedo);

    addGroupSep(mainBar);

    // --- Вид ---
    addGroupLabel(mainBar, tr("Вид"));
    mainBar->addAction(m_actFitAll);
    mainBar->addAction(m_actViewFront);
    mainBar->addAction(m_actViewTop);
    mainBar->addAction(m_actViewRight);
    mainBar->addAction(m_actViewIso);

    addGroupSep(mainBar);

    // --- 2D-эскизы ---
    addGroupLabel(mainBar, tr("Эскиз"));
    mainBar->addAction(m_actSketchRect);
    mainBar->addAction(m_actSketchCircle);
    mainBar->addAction(m_actSketchEllipse);
    mainBar->addAction(m_actSketchPolygon);
    mainBar->addAction(m_actSketchTriangle);

    addGroupSep(mainBar);

    // --- Примитивы (каждый отдельной кнопкой) ---
    addGroupLabel(mainBar, tr("3D тела"));
    mainBar->addAction(m_actBox);
    mainBar->addAction(m_actCylinder);
    mainBar->addAction(m_actSphere);
    mainBar->addAction(m_actCone);
    mainBar->addAction(m_actTorus);
    mainBar->addAction(m_actWedge);

    addGroupSep(mainBar);

    // --- Булевы (выпадающее меню — 3 операции, удобнее в dropdown) ---
    addGroupLabel(mainBar, tr("Булевы"));
    auto* boolMenu = new QMenu(tr("Булевы операции"), this);
    boolMenu->addAction(m_actFuse);
    boolMenu->addAction(m_actCut);
    boolMenu->addAction(m_actCommon);

    auto* boolButton = new QToolButton(this);
    boolButton->setIcon(QIcon(":/icons/bool_fuse"));
    boolButton->setToolTip(tr("Булева операция"));
    boolButton->setPopupMode(QToolButton::InstantPopup);
    boolButton->setMenu(boolMenu);
    boolButton->setIconSize(QSize(iconSz, iconSz));
    mainBar->addWidget(boolButton);

    connect(boolMenu, &QMenu::triggered, this, [boolButton](QAction* action) {
        boolButton->setIcon(action->icon());
    });

    addGroupSep(mainBar);

    // --- Модификации ---
    addGroupLabel(mainBar, tr("Модиф."));
    mainBar->addAction(m_actExtrude);
    mainBar->addAction(m_actFillet);
    mainBar->addAction(m_actChamfer);

    addGroupSep(mainBar);

    // --- Трансформации ---
    addGroupLabel(mainBar, tr("Трансф."));
    mainBar->addAction(m_actTranslate);
    mainBar->addAction(m_actRotate);
    mainBar->addAction(m_actScale);

    addGroupSep(mainBar);

    // --- Инструменты ---
    addGroupLabel(mainBar, tr("Инстр."));
    mainBar->addAction(m_actMeasure);
    mainBar->addAction(m_actPattern);
    mainBar->addAction(m_actMirror);
    mainBar->addAction(m_actDuplicate);

    addGroupSep(mainBar);

    // --- Видимость / Удаление ---
    mainBar->addAction(m_actToggleVisibility);
    mainBar->addAction(m_actDelete);
}

// ============================================================
//  Док-панели
// ============================================================

void MainWindow::createDockWidgets()
{
    QDockWidget* treeDock = new QDockWidget(tr("Дерево модели"), this);
    treeDock->setMinimumWidth(200);
    m_modelTree = new ModelTreeWidget(treeDock);
    treeDock->setWidget(m_modelTree);
    addDockWidget(Qt::LeftDockWidgetArea, treeDock);

    connect(m_modelTree, &ModelTreeWidget::shapeSelected, this, &MainWindow::onTreeShapeSelected);
    connect(m_modelTree, &ModelTreeWidget::deleteRequested, this, [this](int id) {
        if (!m_document) return;
        m_document->removeShape(id);
        m_propertyPanel->clearPanel();
        statusBar()->showMessage(tr("Фигура удалена"));
    });

    QDockWidget* propDock = new QDockWidget(tr("Свойства"), this);
    propDock->setMinimumWidth(200);
    m_propertyPanel = new PropertyPanel(propDock);
    propDock->setWidget(m_propertyPanel);
    addDockWidget(Qt::LeftDockWidgetArea, propDock);
}

// ============================================================
//  Вспомогательные методы
// ============================================================

int MainWindow::requireSelectedShape()
{
    if (!m_document) return -1;
    int id = m_document->selectedShapeId();
    if (id < 0) {
        QMessageBox::information(this, tr("Нет выделения"),
            tr("Сначала выделите фигуру, кликнув на неё в 3D-виде или в дереве модели."));
    }
    return id;
}

void MainWindow::createPrimitive(const QString& type)
{
    if (!m_document) return;

    auto params = PrimitiveDialog::getParameters(type, this);
    if (params.isEmpty()) return;

    TopoDS_Shape shape = Document::rebuildShape(type, params);
    if (shape.IsNull()) return;

    m_document->addShape(type, params, shape);
    m_occView->fitAll();
    statusBar()->showMessage(tr("Создан: %1").arg(type));
}

void MainWindow::doBooleanOp(int boolType)
{
    if (!m_document) return;

    auto bt = static_cast<BooleanType>(boolType);
    int id1, id2;
    if (!BooleanDialog::getTwoShapes(m_document, bt, id1, id2, this)) return;

    int result = m_document->booleanOperation(id1, id2, bt);
    if (result < 0) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Булева операция не удалась."));
    } else {
        m_occView->fitAll();
        statusBar()->showMessage(tr("Булева операция выполнена"));
    }
}

// ============================================================
//  Файл
// ============================================================

void MainWindow::onNewDocument()
{
    if (!m_document) return;
    m_document->clearAll();
    m_propertyPanel->clearPanel();
    statusBar()->showMessage(tr("Новый документ"));
}

void MainWindow::onOpenFile()
{
    QString path = QFileDialog::getOpenFileName(
        this, tr("Импорт файла"), QString(), FileIO::supportedImportFormats());
    if (path.isEmpty()) return;

    if (!m_document) return;
    m_document->importFile(path);
    m_occView->fitAll();
    statusBar()->showMessage(tr("Импортирован: %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::onSaveAs()
{
    if (!m_document || m_document->shapes().isEmpty()) {
        QMessageBox::warning(this, tr("Предупреждение"), tr("Нет фигур для экспорта."));
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this, tr("Экспорт файла"), QString(), FileIO::supportedExportFormats());
    if (path.isEmpty()) return;

    m_document->exportFile(path);
    statusBar()->showMessage(tr("Экспортирован: %1").arg(QFileInfo(path).fileName()));
}

// ============================================================
//  Вид
// ============================================================

void MainWindow::onViewFitAll() { m_occView->fitAll(); }
void MainWindow::onViewFront() { m_occView->viewFront(); }
void MainWindow::onViewTop()   { m_occView->viewTop(); }
void MainWindow::onViewRight() { m_occView->viewRight(); }
void MainWindow::onViewIso()   { m_occView->viewIso(); }

// ============================================================
//  Примитивы
// ============================================================

void MainWindow::onCreateBox()      { createPrimitive("Box"); }
void MainWindow::onCreateCylinder() { createPrimitive("Cylinder"); }
void MainWindow::onCreateSphere()   { createPrimitive("Sphere"); }
void MainWindow::onCreateCone()     { createPrimitive("Cone"); }
void MainWindow::onCreateTorus()    { createPrimitive("Torus"); }
void MainWindow::onCreateWedge()    { createPrimitive("Wedge"); }


// ============================================================
//  Трансформации
// ============================================================

void MainWindow::onTranslate()
{
    int id = requireSelectedShape();
    if (id < 0) return;

    double dx, dy, dz;
    if (TransformDialog::getTranslation(dx, dy, dz, this)) {
        m_document->translateShape(id, dx, dy, dz);
        m_occView->fitAll();
        statusBar()->showMessage(tr("Перемещено на (%1, %2, %3)").arg(dx).arg(dy).arg(dz));
    }
}

void MainWindow::onRotate()
{
    int id = requireSelectedShape();
    if (id < 0) return;

    int axis;
    double angle;
    if (TransformDialog::getRotation(axis, angle, this)) {
        m_document->rotateShape(id, axis, angle);
        m_occView->fitAll();
        QString axisName = (axis == 0) ? "X" : (axis == 1) ? "Y" : "Z";
        statusBar()->showMessage(tr("Повёрнуто на %1 град. вокруг %2").arg(angle).arg(axisName));
    }
}

void MainWindow::onScale()
{
    int id = requireSelectedShape();
    if (id < 0) return;

    double factor;
    if (TransformDialog::getScale(factor, this)) {
        m_document->scaleShape(id, factor);
        m_occView->fitAll();
        statusBar()->showMessage(tr("Масштабировано на %1").arg(factor));
    }
}

// ============================================================
//  Булевы операции
// ============================================================

void MainWindow::onBoolFuse()   { doBooleanOp(static_cast<int>(BooleanType::Fuse)); }
void MainWindow::onBoolCut()    { doBooleanOp(static_cast<int>(BooleanType::Cut)); }
void MainWindow::onBoolCommon() { doBooleanOp(static_cast<int>(BooleanType::Common)); }

// ============================================================
//  Скругление / Фаска
// ============================================================

void MainWindow::onFillet()
{
    int id = requireSelectedShape();
    if (id < 0) return;

    bool ok;
    double radius = QInputDialog::getDouble(this, tr("Скругление"),
        tr("Радиус скругления (мм):"), 5.0, 0.1, 10000.0, 2, &ok);
    if (!ok) return;

    int result = m_document->filletShape(id, radius);
    if (result < 0) {
        QMessageBox::warning(this, tr("Ошибка скругления"),
            tr("Скругление не удалось. Возможно, радиус слишком велик для рёбер фигуры."));
    } else {
        m_occView->fitAll();
        statusBar()->showMessage(tr("Скругление применено (R=%1)").arg(radius));
    }
}

void MainWindow::onChamfer()
{
    int id = requireSelectedShape();
    if (id < 0) return;

    bool ok;
    double dist = QInputDialog::getDouble(this, tr("Фаска"),
        tr("Расстояние фаски (мм):"), 3.0, 0.1, 10000.0, 2, &ok);
    if (!ok) return;

    int result = m_document->chamferShape(id, dist);
    if (result < 0) {
        QMessageBox::warning(this, tr("Ошибка фаски"),
            tr("Фаска не удалась. Возможно, расстояние слишком велико для рёбер фигуры."));
    } else {
        m_occView->fitAll();
        statusBar()->showMessage(tr("Фаска применена (D=%1)").arg(dist));
    }
}

// ============================================================
//  Измерения / Массив / Зеркало / Дублирование
// ============================================================

void MainWindow::onMeasure()
{
    int id = requireSelectedShape();
    if (id < 0) return;

    const ShapeEntry* entry = m_document->findShape(id);
    if (!entry) return;

    MeasureDialog::show(entry->topoShape, entry->name, this);
}

void MainWindow::onPattern()
{
    int id = requireSelectedShape();
    if (id < 0) return;

    bool isCircular;
    int axisIndex, count;
    double step;
    if (!PatternDialog::getParameters(isCircular, axisIndex, step, count, this)) return;

    QList<int> ids = m_document->createPattern(id, isCircular, axisIndex, step, count);
    if (!ids.isEmpty()) {
        m_occView->fitAll();
        statusBar()->showMessage(tr("Массив: создано %1 копий").arg(ids.size()));
    }
}

void MainWindow::onMirror()
{
    int id = requireSelectedShape();
    if (id < 0) return;

    int plane = MirrorDialog::getPlane(this);
    if (plane < 0) return;

    int result = m_document->mirrorShape(id, plane);
    if (result >= 0) {
        m_occView->fitAll();
        statusBar()->showMessage(tr("Зеркальная копия создана"));
    }
}

void MainWindow::onDuplicate()
{
    int id = requireSelectedShape();
    if (id < 0) return;

    int result = m_document->duplicateShape(id);
    if (result >= 0) {
        m_occView->fitAll();
        statusBar()->showMessage(tr("Фигура дублирована"));
    }
}

void MainWindow::onToggleVisibility()
{
    if (!m_document) return;
    int id = m_document->selectedShapeId();
    if (id < 0) return;

    ShapeEntry* entry = m_document->findShape(id);
    if (!entry) return;

    m_document->setShapeVisible(id, !entry->visible);
}

// ============================================================
//  2D-эскизы
// ============================================================

void MainWindow::createSketch(const QString& type)
{
    if (!m_document) return;

    int planeIdx = 0;
    double planeOfs = 0.0;
    auto params = SketchDialog::getParameters(type, planeIdx, planeOfs, this);
    if (params.isEmpty()) return;

    TopoDS_Shape face = Document::rebuildSketch(type, params, planeIdx, planeOfs);
    if (face.IsNull()) return;

    m_document->addSketch(type, params, planeIdx, planeOfs, face);
    m_occView->fitAll();
    statusBar()->showMessage(tr("Эскиз создан: %1").arg(type));
}

void MainWindow::onSketchRectangle() { createSketch("Sketch_Rectangle"); }
void MainWindow::onSketchCircle()    { createSketch("Sketch_Circle"); }
void MainWindow::onSketchEllipse()   { createSketch("Sketch_Ellipse"); }
void MainWindow::onSketchPolygon()   { createSketch("Sketch_Polygon"); }
void MainWindow::onSketchTriangle()  { createSketch("Sketch_Triangle"); }

void MainWindow::onExtrude()
{
    int id = requireSelectedShape();
    if (id < 0) return;

    const ShapeEntry* entry = m_document->findShape(id);
    if (!entry || !Document::isSketchType(entry->type)) {
        QMessageBox::information(this, tr("Выдавливание"),
            tr("Выберите 2D-эскиз для выдавливания.\n"
               "Создайте эскиз через меню Эскиз или панель инструментов."));
        return;
    }

    double height = 50.0;
    bool symmetric = true;
    if (!ExtrudeDialog::getParameters(height, symmetric, this)) return;

    int result = m_document->extrudeShape(id, height, symmetric);
    if (result < 0) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Выдавливание не удалось."));
    } else {
        m_occView->fitAll();
        statusBar()->showMessage(tr("Выдавливание выполнено (H=%1)").arg(height));
    }
}

// ============================================================
//  Удаление
// ============================================================

void MainWindow::onDeleteSelected()
{
    int id = requireSelectedShape();
    if (id < 0) return;

    m_document->removeShape(id);
    m_propertyPanel->clearPanel();
    statusBar()->showMessage(tr("Фигура удалена"));
}

// ============================================================
//  Синхронизация выделения
// ============================================================

void MainWindow::onViewerSelectionChanged()
{
    if (!m_document) return;
    m_document->syncSelectionFromViewer();
}

void MainWindow::onTreeShapeSelected(int id)
{
    if (!m_document) return;

    ShapeEntry* entry = m_document->findShape(id);
    if (entry) {
        m_document->context()->ClearSelected(Standard_False);
        m_document->context()->AddOrRemoveSelected(entry->aisShape, Standard_True);
    }

    onSelectionChanged(id);
}

void MainWindow::onSelectionChanged(int id)
{
    m_modelTree->selectShapeById(id);
    m_propertyPanel->showShape(id);
}
