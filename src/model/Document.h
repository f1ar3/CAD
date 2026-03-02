#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QString>
#include <QColor>

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Trsf.hxx>

#include "../commands/CommandHistory.h"

/**
 * @brief Запись об одной фигуре в документе.
 *
 * Содержит всю информацию о фигуре: идентификатор, имя, тип,
 * параметры построения, цвет, а также ссылки на OpenCascade-объекты
 * (AIS_Shape для отображения и TopoDS_Shape для геометрии).
 */
struct ShapeEntry {
    int id;                        ///< Уникальный идентификатор
    QString name;                  ///< Имя фигуры (например, "Box_1")
    QString type;                  ///< Тип: Box, Cylinder, Sphere, Cone, Torus, Wedge, Import и др.
    QMap<QString, double> params;  ///< Параметры построения (англ. ключи)
    QColor color = QColor(180, 180, 220); ///< Цвет фигуры (по умолчанию серо-голубой)
    double posX = 0.0;             ///< Позиция X в мировых координатах (мм)
    double posY = 0.0;             ///< Позиция Y в мировых координатах (мм)
    double posZ = 0.0;             ///< Позиция Z в мировых координатах (мм)
    bool visible = true;           ///< Видимость фигуры в 3D-виде
    Handle(AIS_Shape) aisShape;    ///< Интерактивный объект для 3D-отображения
    TopoDS_Shape topoShape;        ///< Топологическая геометрия OpenCascade
};

/**
 * @brief Тип булевой операции.
 */
enum class BooleanType {
    Fuse,   ///< Объединение
    Cut,    ///< Вычитание
    Common  ///< Пересечение
};

/**
 * @brief Основной класс модели документа САПР.
 *
 * Управляет коллекцией фигур, их отображением в AIS_InteractiveContext,
 * трансформациями, булевыми операциями, параметрическим редактированием,
 * а также историей операций (undo/redo) через snapshot-механизм.
 *
 * Каждая мутирующая операция сохраняет текущее состояние в undo-стек
 * перед выполнением изменений.
 */
class Document : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Создать документ с привязкой к AIS-контексту.
     * @param context Контекст OpenCascade для отображения фигур
     * @param parent Родительский QObject
     */
    explicit Document(Handle(AIS_InteractiveContext) context, QObject* parent = nullptr);
    ~Document() override;

    // --- Управление фигурами ---

    /**
     * @brief Добавить примитивную фигуру.
     * @param type Тип фигуры (Box, Cylinder и т.д.)
     * @param params Параметры построения
     * @param shape Готовая геометрия
     * @return ID добавленной фигуры
     */
    int addShape(const QString& type, const QMap<QString, double>& params,
                 const TopoDS_Shape& shape);

    /**
     * @brief Добавить импортированную фигуру.
     * @param name Имя файла (без расширения)
     * @param shape Импортированная геометрия
     * @return ID добавленной фигуры
     */
    int addImportedShape(const QString& name, const TopoDS_Shape& shape);

    /** @brief Удалить фигуру по ID. */
    void removeShape(int id);

    /** @brief Удалить все фигуры и сбросить счётчики. */
    void clearAll();

    // --- Запросы ---

    /** @brief Список всех фигур (только для чтения). */
    const QList<ShapeEntry>& shapes() const { return m_shapes; }

    /** @brief Найти фигуру по ID. @return nullptr если не найдена. */
    ShapeEntry* findShape(int id);

    /** @brief Найти фигуру по ID (const-версия). */
    const ShapeEntry* findShape(int id) const;

    /** @brief ID первой выделенной фигуры или -1. */
    int selectedShapeId() const;

    /** @brief Список ID всех выделенных фигур. */
    QList<int> selectedShapeIds() const;

    // --- Трансформации ---

    /** @brief Переместить фигуру на (dx, dy, dz) мм. */
    void translateShape(int id, double dx, double dy, double dz);

    /**
     * @brief Повернуть фигуру вокруг оси.
     * @param id ID фигуры
     * @param axisIndex 0=X, 1=Y, 2=Z
     * @param angleDeg Угол в градусах
     */
    void rotateShape(int id, int axisIndex, double angleDeg);

    /** @brief Масштабировать фигуру. */
    void scaleShape(int id, double factor);

    // --- Булевы операции ---

    /**
     * @brief Выполнить булеву операцию над двумя фигурами.
     *
     * Исходные фигуры удаляются, результат добавляется как новая фигура.
     * @return ID результата или -1 при ошибке
     */
    int booleanOperation(int id1, int id2, BooleanType type);

    // --- Параметрическое редактирование ---

    /**
     * @brief Перестроить геометрию фигуры с новыми параметрами.
     *
     * Работает только для примитивных типов (Box, Cylinder и т.д.).
     */
    void updateShapeParams(int id, const QMap<QString, double>& newParams);

    /** @brief Изменить цвет фигуры. */
    void setShapeColor(int id, const QColor& color);

    /** @brief Переименовать фигуру. */
    void renameShape(int id, const QString& newName);

    /** @brief Установить абсолютную позицию фигуры в мировых координатах. */
    void setShapePosition(int id, double x, double y, double z);

    /** @brief Установить видимость фигуры. */
    void setShapeVisible(int id, bool visible);

    /**
     * @brief Построить TopoDS_Shape из типа и параметров.
     *
     * Статический метод, не требует экземпляра Document.
     * Поддерживаемые типы: Box, Cylinder, Sphere, Cone, Torus, Wedge.
     * @return Построенная геометрия или IsNull() для неизвестных типов
     */
    static TopoDS_Shape rebuildShape(const QString& type, const QMap<QString, double>& params);

    // --- Скругление / Фаска ---

    /**
     * @brief Скруглить все рёбра фигуры.
     * @return ID новой фигуры или -1 при ошибке
     */
    int filletShape(int id, double radius);

    /**
     * @brief Снять фаску со всех рёбер фигуры.
     * @return ID новой фигуры или -1 при ошибке
     */
    int chamferShape(int id, double distance);

    // --- Undo / Redo ---

    /** @brief Отменить последнюю операцию. */
    void undo();

    /** @brief Повторить отменённую операцию. */
    void redo();

    /** @brief Доступна ли отмена. */
    bool canUndo() const;

    /** @brief Доступен ли повтор. */
    bool canRedo() const;

    /** @brief Описание операции для отмены. */
    QString undoDescription() const;

    /** @brief Описание операции для повтора. */
    QString redoDescription() const;

    // --- Прочее ---

    /** @brief Синхронизировать выделение из 3D-вида в модель. */
    void syncSelectionFromViewer();

    /** @brief Импортировать файл STEP/IGES/BRep. */
    void importFile(const QString& path);

    /** @brief Экспортировать все фигуры в файл. */
    void exportFile(const QString& path);

    /** @brief Получить AIS-контекст. */
    Handle(AIS_InteractiveContext) context() const { return m_context; }

signals:
    void shapeAdded(int id);     ///< Фигура добавлена
    void shapeRemoved(int id);   ///< Фигура удалена
    void selectionChanged(int id); ///< Изменилось выделение
    void modelChanged();          ///< Модель изменилась (любое изменение)
    void undoRedoChanged();       ///< Изменилось состояние undo/redo

private:
    Snapshot makeSnapshot(const QString& description) const;
    void saveSnapshot(const QString& description);
    void restoreSnapshot(const Snapshot& snapshot);
    TopoDS_Shape buildCompound() const;
    QString generateName(const QString& type);

    Handle(AIS_InteractiveContext) m_context;
    QList<ShapeEntry> m_shapes;
    int m_nextId = 1;
    QMap<QString, int> m_nameCounters;
    CommandHistory m_history;
};

#endif // DOCUMENT_H
