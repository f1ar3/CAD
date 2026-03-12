#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class OccView;
class Document;
class ModelTreeWidget;
class PropertyPanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    // File
    void onNewDocument();
    void onOpenFile();
    void onSaveAs();

    // View
    void onViewFitAll();
    void onViewFront();
    void onViewTop();
    void onViewRight();
    void onViewIso();

    // Primitives
    void onCreateBox();
    void onCreateCylinder();
    void onCreateSphere();
    void onCreateCone();
    void onCreateTorus();
    void onCreateWedge();

    // Transform
    void onTranslate();
    void onRotate();
    void onScale();

    // Boolean
    void onBoolFuse();
    void onBoolCut();
    void onBoolCommon();

    // Fillet/Chamfer
    void onFillet();
    void onChamfer();

    // Measure / Pattern / Mirror
    void onMeasure();
    void onPattern();
    void onMirror();
    void onDuplicate();
    void onToggleVisibility();

    // Sketches
    void onSketchRectangle();
    void onSketchCircle();
    void onSketchEllipse();
    void onSketchPolygon();
    void onSketchTriangle();
    void onExtrude();

    // Delete
    void onDeleteSelected();

    // Selection sync
    void onViewerSelectionChanged();
    void onTreeShapeSelected(int id);
    void onSelectionChanged(int id);

private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createDockWidgets();
    void initDocument();

    int requireSelectedShape();
    void createPrimitive(const QString& type);
    void createSketch(const QString& type);
    void doBooleanOp(int boolType);

    OccView*         m_occView = nullptr;
    Document*        m_document = nullptr;
    ModelTreeWidget* m_modelTree = nullptr;
    PropertyPanel*   m_propertyPanel = nullptr;

    // File actions
    QAction* m_actNew = nullptr;
    QAction* m_actOpen = nullptr;
    QAction* m_actSaveAs = nullptr;
    QAction* m_actExit = nullptr;

    // Edit actions
    QAction* m_actUndo = nullptr;
    QAction* m_actRedo = nullptr;
    QAction* m_actDelete = nullptr;

    // View actions
    QAction* m_actFitAll = nullptr;
    QAction* m_actViewFront = nullptr;
    QAction* m_actViewTop = nullptr;
    QAction* m_actViewRight = nullptr;
    QAction* m_actViewIso = nullptr;

    // Primitive actions
    QAction* m_actBox = nullptr;
    QAction* m_actCylinder = nullptr;
    QAction* m_actSphere = nullptr;
    QAction* m_actCone = nullptr;
    QAction* m_actTorus = nullptr;
    QAction* m_actWedge = nullptr;

    // Transform actions
    QAction* m_actTranslate = nullptr;
    QAction* m_actRotate = nullptr;
    QAction* m_actScale = nullptr;

    // Boolean actions
    QAction* m_actFuse = nullptr;
    QAction* m_actCut = nullptr;
    QAction* m_actCommon = nullptr;

    // Fillet/Chamfer actions
    QAction* m_actFillet = nullptr;
    QAction* m_actChamfer = nullptr;

    // Measure / Pattern / Mirror / Duplicate / Visibility
    QAction* m_actMeasure = nullptr;
    QAction* m_actPattern = nullptr;
    QAction* m_actMirror = nullptr;
    QAction* m_actDuplicate = nullptr;
    QAction* m_actToggleVisibility = nullptr;

    // Sketch actions
    QAction* m_actSketchRect = nullptr;
    QAction* m_actSketchCircle = nullptr;
    QAction* m_actSketchEllipse = nullptr;
    QAction* m_actSketchPolygon = nullptr;
    QAction* m_actSketchTriangle = nullptr;
    QAction* m_actExtrude = nullptr;

    // Drag state
    int m_draggedShapeId = -1;
};

#endif // MAINWINDOW_H
