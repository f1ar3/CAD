#ifndef COMMANDHISTORY_H
#define COMMANDHISTORY_H

#include <QList>
#include <QMap>
#include <QString>
#include <QColor>

#include <TopoDS_Shape.hxx>

/**
 * @brief Снимок состояния документа для undo/redo.
 *
 * Хранит полное состояние всех фигур на момент операции,
 * включая геометрию, параметры, цвет и счётчики имён.
 */
struct Snapshot {
    QString description; ///< Описание операции (для отображения в UI)

    /**
     * @brief Данные одной фигуры внутри снимка.
     */
    struct ShapeData {
        int id;                        ///< Уникальный идентификатор фигуры
        QString name;                  ///< Имя фигуры
        QString type;                  ///< Тип (Box, Cylinder, Sphere и т.д.)
        QMap<QString, double> params;  ///< Параметры фигуры (ключ — англ. имя)
        QColor color;                  ///< Цвет фигуры
        double posX = 0.0;            ///< Позиция X
        double posY = 0.0;            ///< Позиция Y
        double posZ = 0.0;            ///< Позиция Z
        bool visible = true;          ///< Видимость
        double transparency = 0.0;    ///< Прозрачность
        int displayMode = 1;          ///< Режим отображения
        TopoDS_Shape topoShape;        ///< Геометрия OpenCascade
    };

    QList<ShapeData> shapes;           ///< Список всех фигур
    int nextId = 1;                    ///< Следующий свободный ID
    QMap<QString, int> nameCounters;   ///< Счётчики для генерации имён
};

/**
 * @brief Менеджер истории операций (undo/redo).
 *
 * Реализует двухстековую схему: undo-стек и redo-стек.
 * При каждой мутирующей операции текущее состояние
 * сохраняется в undo-стек. Максимальная глубина — 50 шагов.
 */
class CommandHistory
{
public:
    /** @brief Сохранить снимок в undo-стек. Если стек превышает MAX_HISTORY, удаляется самый старый. */
    void pushUndo(const Snapshot& snapshot);

    /** @brief Сохранить снимок в redo-стек. */
    void pushRedo(const Snapshot& snapshot);

    /** @brief Есть ли доступные операции для отмены. */
    bool canUndo() const;

    /** @brief Есть ли доступные операции для повтора. */
    bool canRedo() const;

    /** @brief Извлечь последний снимок из undo-стека. */
    Snapshot popUndo();

    /** @brief Извлечь последний снимок из redo-стека. */
    Snapshot popRedo();

    /** @brief Очистить redo-стек (вызывается при новой операции). */
    void clearRedo();

    /** @brief Очистить оба стека. */
    void clearAll();

    /** @brief Описание последней операции в undo-стеке. */
    QString undoDescription() const;

    /** @brief Описание последней операции в redo-стеке. */
    QString redoDescription() const;

private:
    static const int MAX_HISTORY = 50; ///< Максимальная глубина истории

    QList<Snapshot> m_undoStack; ///< Стек отмены
    QList<Snapshot> m_redoStack; ///< Стек повтора
};

#endif // COMMANDHISTORY_H
