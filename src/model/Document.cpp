#include "Document.h"
#include "../io/FileIO.h"

#include <QFileInfo>
#include <QMessageBox>

#include <BRep_Builder.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepPrimAPI_MakeWedge.hxx>
#include <Quantity_Color.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Ax1.hxx>

Document::Document(Handle(AIS_InteractiveContext) context, QObject* parent)
    : QObject(parent)
    , m_context(context)
{
}

Document::~Document() = default;

// ============================================================
//  Snapshot / Undo / Redo
// ============================================================

Snapshot Document::makeSnapshot(const QString& description) const
{
    Snapshot snap;
    snap.description = description;
    snap.nextId = m_nextId;
    snap.nameCounters = m_nameCounters;

    for (const auto& entry : m_shapes) {
        Snapshot::ShapeData sd;
        sd.id = entry.id;
        sd.name = entry.name;
        sd.type = entry.type;
        sd.params = entry.params;
        sd.color = entry.color;
        sd.topoShape = entry.topoShape;
        snap.shapes.append(sd);
    }
    return snap;
}

void Document::saveSnapshot(const QString& description)
{
    m_history.pushUndo(makeSnapshot(description));
    m_history.clearRedo();
    emit undoRedoChanged();
}

void Document::restoreSnapshot(const Snapshot& snapshot)
{
    // Remove all current AIS shapes from context
    for (auto& entry : m_shapes) {
        m_context->Remove(entry.aisShape, Standard_False);
    }
    m_shapes.clear();

    // Restore state
    m_nextId = snapshot.nextId;
    m_nameCounters = snapshot.nameCounters;

    for (const auto& sd : snapshot.shapes) {
        ShapeEntry entry;
        entry.id = sd.id;
        entry.name = sd.name;
        entry.type = sd.type;
        entry.params = sd.params;
        entry.color = sd.color;
        entry.topoShape = sd.topoShape;
        entry.aisShape = new AIS_Shape(sd.topoShape);
        m_context->Display(entry.aisShape, Standard_False);
        m_context->SetColor(entry.aisShape,
            Quantity_Color(entry.color.redF(), entry.color.greenF(), entry.color.blueF(),
                           Quantity_TOC_RGB),
            Standard_False);
        m_shapes.append(entry);
    }

    m_context->UpdateCurrentViewer();
    emit modelChanged();
    emit undoRedoChanged();
}

void Document::undo()
{
    if (!m_history.canUndo()) return;

    // Save current state to redo stack
    Snapshot current = makeSnapshot(m_history.undoDescription());
    m_history.pushRedo(current);

    // Restore previous state
    Snapshot prev = m_history.popUndo();
    restoreSnapshot(prev);
}

void Document::redo()
{
    if (!m_history.canRedo()) return;

    // Save current state to undo stack
    Snapshot current = makeSnapshot(m_history.redoDescription());
    m_history.pushUndo(current);

    // Restore next state
    Snapshot next = m_history.popRedo();
    restoreSnapshot(next);
}

bool Document::canUndo() const { return m_history.canUndo(); }
bool Document::canRedo() const { return m_history.canRedo(); }
QString Document::undoDescription() const { return m_history.undoDescription(); }
QString Document::redoDescription() const { return m_history.redoDescription(); }

// ============================================================
//  Shape Management
// ============================================================

QString Document::generateName(const QString& type)
{
    int& counter = m_nameCounters[type];
    counter++;
    return QString("%1_%2").arg(type).arg(counter);
}

int Document::addShape(const QString& type, const QMap<QString, double>& params,
                       const TopoDS_Shape& shape)
{
    saveSnapshot(QString::fromUtf8("Создан: %1").arg(type));

    ShapeEntry entry;
    entry.id = m_nextId++;
    entry.name = generateName(type);
    entry.type = type;
    entry.params = params;
    entry.topoShape = shape;
    entry.aisShape = new AIS_Shape(shape);

    m_context->Display(entry.aisShape, Standard_False);
    m_context->SetColor(entry.aisShape,
        Quantity_Color(entry.color.redF(), entry.color.greenF(), entry.color.blueF(),
                       Quantity_TOC_RGB),
        Standard_False);
    m_shapes.append(entry);
    m_context->UpdateCurrentViewer();

    emit shapeAdded(entry.id);
    emit modelChanged();
    return entry.id;
}

int Document::addImportedShape(const QString& name, const TopoDS_Shape& shape)
{
    saveSnapshot(QString::fromUtf8("Импорт: %1").arg(name));

    ShapeEntry entry;
    entry.id = m_nextId++;
    entry.name = name;
    entry.type = "Import";
    entry.topoShape = shape;
    entry.aisShape = new AIS_Shape(shape);

    m_context->Display(entry.aisShape, Standard_False);
    m_shapes.append(entry);
    m_context->UpdateCurrentViewer();

    emit shapeAdded(entry.id);
    emit modelChanged();
    return entry.id;
}

void Document::removeShape(int id)
{
    ShapeEntry* entry = findShape(id);
    if (!entry) return;

    saveSnapshot(QString::fromUtf8("Удалено: %1").arg(entry->name));

    for (int i = 0; i < m_shapes.size(); ++i) {
        if (m_shapes[i].id == id) {
            m_context->Remove(m_shapes[i].aisShape, Standard_False);
            m_shapes.removeAt(i);
            m_context->UpdateCurrentViewer();
            emit shapeRemoved(id);
            emit modelChanged();
            return;
        }
    }
}

void Document::clearAll()
{
    if (!m_shapes.isEmpty()) {
        saveSnapshot(QString::fromUtf8("Очистка"));
    }

    for (auto& entry : m_shapes) {
        m_context->Remove(entry.aisShape, Standard_False);
    }
    m_shapes.clear();
    m_nameCounters.clear();
    m_nextId = 1;
    m_context->UpdateCurrentViewer();
    emit modelChanged();
}

ShapeEntry* Document::findShape(int id)
{
    for (auto& entry : m_shapes) {
        if (entry.id == id) return &entry;
    }
    return nullptr;
}

const ShapeEntry* Document::findShape(int id) const
{
    for (const auto& entry : m_shapes) {
        if (entry.id == id) return &entry;
    }
    return nullptr;
}

int Document::selectedShapeId() const
{
    m_context->InitSelected();
    if (m_context->MoreSelected()) {
        Handle(AIS_InteractiveObject) obj = m_context->SelectedInteractive();
        for (const auto& entry : m_shapes) {
            if (entry.aisShape == obj) return entry.id;
        }
    }
    return -1;
}

QList<int> Document::selectedShapeIds() const
{
    QList<int> ids;
    for (m_context->InitSelected(); m_context->MoreSelected(); m_context->NextSelected()) {
        Handle(AIS_InteractiveObject) obj = m_context->SelectedInteractive();
        for (const auto& entry : m_shapes) {
            if (entry.aisShape == obj) {
                ids.append(entry.id);
                break;
            }
        }
    }
    return ids;
}

void Document::syncSelectionFromViewer()
{
    int id = selectedShapeId();
    emit selectionChanged(id);
}

// ============================================================
//  Transformations
// ============================================================

void Document::translateShape(int id, double dx, double dy, double dz)
{
    ShapeEntry* entry = findShape(id);
    if (!entry) return;

    saveSnapshot(QString::fromUtf8("Перемещение: %1").arg(entry->name));

    gp_Trsf trsf;
    trsf.SetTranslation(gp_Vec(dx, dy, dz));
    BRepBuilderAPI_Transform transform(entry->topoShape, trsf, Standard_True);
    entry->topoShape = transform.Shape();

    m_context->Remove(entry->aisShape, Standard_False);
    entry->aisShape = new AIS_Shape(entry->topoShape);
    m_context->Display(entry->aisShape, Standard_True);

    emit modelChanged();
}

void Document::rotateShape(int id, int axisIndex, double angleDeg)
{
    ShapeEntry* entry = findShape(id);
    if (!entry) return;

    saveSnapshot(QString::fromUtf8("Вращение: %1").arg(entry->name));

    gp_Ax1 axis;
    switch (axisIndex) {
    case 0: axis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(1, 0, 0)); break;
    case 1: axis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0)); break;
    case 2: axis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)); break;
    default: return;
    }

    gp_Trsf trsf;
    trsf.SetRotation(axis, angleDeg * M_PI / 180.0);
    BRepBuilderAPI_Transform transform(entry->topoShape, trsf, Standard_True);
    entry->topoShape = transform.Shape();

    m_context->Remove(entry->aisShape, Standard_False);
    entry->aisShape = new AIS_Shape(entry->topoShape);
    m_context->Display(entry->aisShape, Standard_True);

    emit modelChanged();
}

void Document::scaleShape(int id, double factor)
{
    ShapeEntry* entry = findShape(id);
    if (!entry) return;

    saveSnapshot(QString::fromUtf8("Масштабирование: %1").arg(entry->name));

    gp_Trsf trsf;
    trsf.SetScaleFactor(factor);
    BRepBuilderAPI_Transform transform(entry->topoShape, trsf, Standard_True);
    entry->topoShape = transform.Shape();

    m_context->Remove(entry->aisShape, Standard_False);
    entry->aisShape = new AIS_Shape(entry->topoShape);
    m_context->Display(entry->aisShape, Standard_True);

    emit modelChanged();
}

// ============================================================
//  Boolean Operations
// ============================================================

int Document::booleanOperation(int id1, int id2, BooleanType type)
{
    ShapeEntry* s1 = findShape(id1);
    ShapeEntry* s2 = findShape(id2);
    if (!s1 || !s2) return -1;

    QString typeName;
    switch (type) {
    case BooleanType::Fuse:   typeName = QString::fromUtf8("Объединение"); break;
    case BooleanType::Cut:    typeName = QString::fromUtf8("Вычитание"); break;
    case BooleanType::Common: typeName = QString::fromUtf8("Пересечение"); break;
    }

    saveSnapshot(typeName);

    TopoDS_Shape result;
    switch (type) {
    case BooleanType::Fuse: {
        BRepAlgoAPI_Fuse fuse(s1->topoShape, s2->topoShape);
        if (!fuse.IsDone()) return -1;
        result = fuse.Shape();
        break;
    }
    case BooleanType::Cut: {
        BRepAlgoAPI_Cut cut(s1->topoShape, s2->topoShape);
        if (!cut.IsDone()) return -1;
        result = cut.Shape();
        break;
    }
    case BooleanType::Common: {
        BRepAlgoAPI_Common common(s1->topoShape, s2->topoShape);
        if (!common.IsDone()) return -1;
        result = common.Shape();
        break;
    }
    }

    // Remove source shapes (without saving snapshot again)
    for (int i = m_shapes.size() - 1; i >= 0; --i) {
        if (m_shapes[i].id == id1 || m_shapes[i].id == id2) {
            m_context->Remove(m_shapes[i].aisShape, Standard_False);
            m_shapes.removeAt(i);
        }
    }

    // Add result (without saving snapshot again)
    ShapeEntry entry;
    entry.id = m_nextId++;
    entry.name = generateName(typeName);
    entry.type = typeName;
    entry.topoShape = result;
    entry.aisShape = new AIS_Shape(result);

    m_context->Display(entry.aisShape, Standard_False);
    m_shapes.append(entry);
    m_context->UpdateCurrentViewer();

    emit modelChanged();
    return entry.id;
}

// ============================================================
//  Fillet / Chamfer
// ============================================================

int Document::filletShape(int id, double radius)
{
    ShapeEntry* entry = findShape(id);
    if (!entry) return -1;

    BRepFilletAPI_MakeFillet fillet(entry->topoShape);
    for (TopExp_Explorer exp(entry->topoShape, TopAbs_EDGE); exp.More(); exp.Next()) {
        fillet.Add(radius, TopoDS::Edge(exp.Current()));
    }

    try {
        fillet.Build();
        if (!fillet.IsDone()) return -1;
    } catch (...) {
        return -1;
    }

    saveSnapshot(QString::fromUtf8("Скругление: %1").arg(entry->name));

    TopoDS_Shape result = fillet.Shape();
    QString oldName = entry->name;

    // Remove old shape (no extra snapshot)
    for (int i = 0; i < m_shapes.size(); ++i) {
        if (m_shapes[i].id == id) {
            m_context->Remove(m_shapes[i].aisShape, Standard_False);
            m_shapes.removeAt(i);
            break;
        }
    }

    // Add result (no extra snapshot)
    ShapeEntry newEntry;
    newEntry.id = m_nextId++;
    newEntry.name = QString::fromUtf8("Скругление(%1)").arg(oldName);
    newEntry.type = QString::fromUtf8("Скругление");
    newEntry.params["radius"] = radius;
    newEntry.topoShape = result;
    newEntry.aisShape = new AIS_Shape(result);

    m_context->Display(newEntry.aisShape, Standard_False);
    m_shapes.append(newEntry);
    m_context->UpdateCurrentViewer();

    emit modelChanged();
    return newEntry.id;
}

int Document::chamferShape(int id, double distance)
{
    ShapeEntry* entry = findShape(id);
    if (!entry) return -1;

    BRepFilletAPI_MakeChamfer chamfer(entry->topoShape);
    for (TopExp_Explorer exp(entry->topoShape, TopAbs_EDGE); exp.More(); exp.Next()) {
        chamfer.Add(distance, TopoDS::Edge(exp.Current()));
    }

    try {
        chamfer.Build();
        if (!chamfer.IsDone()) return -1;
    } catch (...) {
        return -1;
    }

    saveSnapshot(QString::fromUtf8("Фаска: %1").arg(entry->name));

    TopoDS_Shape result = chamfer.Shape();
    QString oldName = entry->name;

    // Remove old shape (no extra snapshot)
    for (int i = 0; i < m_shapes.size(); ++i) {
        if (m_shapes[i].id == id) {
            m_context->Remove(m_shapes[i].aisShape, Standard_False);
            m_shapes.removeAt(i);
            break;
        }
    }

    // Add result (no extra snapshot)
    ShapeEntry newEntry;
    newEntry.id = m_nextId++;
    newEntry.name = QString::fromUtf8("Фаска(%1)").arg(oldName);
    newEntry.type = QString::fromUtf8("Фаска");
    newEntry.params["distance"] = distance;
    newEntry.topoShape = result;
    newEntry.aisShape = new AIS_Shape(result);

    m_context->Display(newEntry.aisShape, Standard_False);
    m_shapes.append(newEntry);
    m_context->UpdateCurrentViewer();

    emit modelChanged();
    return newEntry.id;
}

// ============================================================
//  Parametric Editing
// ============================================================

TopoDS_Shape Document::rebuildShape(const QString& type, const QMap<QString, double>& params)
{
    TopoDS_Shape shape;

    if (type == "Box") {
        shape = BRepPrimAPI_MakeBox(
            params["Width (X)"], params["Depth (Y)"], params["Height (Z)"]).Shape();
    } else if (type == "Cylinder") {
        shape = BRepPrimAPI_MakeCylinder(params["Radius"], params["Height"]).Shape();
    } else if (type == "Sphere") {
        shape = BRepPrimAPI_MakeSphere(params["Radius"]).Shape();
    } else if (type == "Cone") {
        shape = BRepPrimAPI_MakeCone(
            params["Bottom Radius"], params["Top Radius"], params["Height"]).Shape();
    } else if (type == "Torus") {
        shape = BRepPrimAPI_MakeTorus(params["Major Radius"], params["Minor Radius"]).Shape();
    } else if (type == "Wedge") {
        shape = BRepPrimAPI_MakeWedge(
            params["Width (X)"], params["Height (Z)"], params["Depth (Y)"],
            params["Top Width (Xmin)"]).Shape();
    }

    return shape;
}

void Document::updateShapeParams(int id, const QMap<QString, double>& newParams)
{
    ShapeEntry* entry = findShape(id);
    if (!entry) return;

    TopoDS_Shape newShape = rebuildShape(entry->type, newParams);
    if (newShape.IsNull()) return;

    saveSnapshot(QString::fromUtf8("Изменение параметров: %1").arg(entry->name));

    entry->params = newParams;
    entry->topoShape = newShape;

    m_context->Remove(entry->aisShape, Standard_False);
    entry->aisShape = new AIS_Shape(newShape);
    m_context->Display(entry->aisShape, Standard_False);
    m_context->SetColor(entry->aisShape,
        Quantity_Color(entry->color.redF(), entry->color.greenF(), entry->color.blueF(),
                       Quantity_TOC_RGB),
        Standard_False);
    m_context->UpdateCurrentViewer();

    emit modelChanged();
}

void Document::setShapeColor(int id, const QColor& color)
{
    ShapeEntry* entry = findShape(id);
    if (!entry) return;

    saveSnapshot(QString::fromUtf8("Цвет: %1").arg(entry->name));

    entry->color = color;
    m_context->SetColor(entry->aisShape,
        Quantity_Color(color.redF(), color.greenF(), color.blueF(), Quantity_TOC_RGB),
        Standard_True);

    emit modelChanged();
}

void Document::renameShape(int id, const QString& newName)
{
    ShapeEntry* entry = findShape(id);
    if (!entry || newName.isEmpty()) return;

    saveSnapshot(QString::fromUtf8("Переименование: %1").arg(entry->name));

    entry->name = newName;
    emit modelChanged();
}

// ============================================================
//  I/O
// ============================================================

void Document::importFile(const QString& path)
{
    QFileInfo fi(path);
    QString ext = fi.suffix().toLower();

    TopoDS_Shape shape;
    try {
        if (ext == "step" || ext == "stp") {
            shape = FileIO::importSTEP(path);
        } else if (ext == "iges" || ext == "igs") {
            shape = FileIO::importIGES(path);
        } else if (ext == "brep" || ext == "brp") {
            shape = FileIO::importBRep(path);
        } else {
            throw std::runtime_error("Неподдерживаемый формат: " + ext.toStdString());
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, tr("Ошибка импорта"), QString::fromStdString(e.what()));
        return;
    }

    addImportedShape(fi.baseName(), shape);
}

void Document::exportFile(const QString& path)
{
    if (m_shapes.isEmpty()) return;

    TopoDS_Shape compound = buildCompound();

    QFileInfo fi(path);
    QString ext = fi.suffix().toLower();

    bool ok = false;
    if (ext == "step" || ext == "stp") {
        ok = FileIO::exportSTEP(compound, path);
    } else if (ext == "iges" || ext == "igs") {
        ok = FileIO::exportIGES(compound, path);
    } else if (ext == "brep" || ext == "brp") {
        ok = FileIO::exportBRep(compound, path);
    }

    if (!ok) {
        QMessageBox::critical(nullptr, tr("Ошибка экспорта"), tr("Не удалось экспортировать файл."));
    }
}

TopoDS_Shape Document::buildCompound() const
{
    TopoDS_Compound compound;
    BRep_Builder builder;
    builder.MakeCompound(compound);
    for (const auto& entry : m_shapes) {
        builder.Add(compound, entry.topoShape);
    }
    return compound;
}
