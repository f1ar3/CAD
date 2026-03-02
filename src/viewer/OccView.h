#ifndef OCCVIEW_H
#define OCCVIEW_H

#include <QWidget>

#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

/**
 * @brief Виджет 3D-отображения на базе OpenCascade V3d.
 *
 * Встраивает OpenCascade V3d_View в QWidget через Cocoa_Window (macOS).
 * Обрабатывает мышь: среднее колесо — вращение, Shift+среднее — панорама,
 * скролл — зум, ЛКМ — выделение, Cmd/Shift+ЛКМ — мультивыделение.
 *
 * paintEngine() возвращает nullptr, т.к. рисование выполняется OpenGL
 * напрямую через OpenCascade.
 */
class OccView : public QWidget
{
    Q_OBJECT

public:
    explicit OccView(QWidget* parent = nullptr);
    ~OccView() override;

    /** @brief Получить AIS-контекст для управления отображаемыми объектами. */
    Handle(AIS_InteractiveContext) context() const { return m_context; }

    /** @brief Получить 3D-вид. */
    Handle(V3d_View) view() const { return m_view; }

    /** @brief Вписать все объекты в окно. */
    void fitAll();

    /** @brief Установить вид спереди (вдоль оси Y). */
    void viewFront();

    /** @brief Установить вид сверху (вдоль оси Z). */
    void viewTop();

    /** @brief Установить вид справа (вдоль оси X). */
    void viewRight();

    /** @brief Установить изометрический вид. */
    void viewIso();

    /** @brief Возвращает nullptr — рисование через OpenGL. */
    QPaintEngine* paintEngine() const override { return nullptr; }

signals:
    /** @brief Изменилось выделение объектов в 3D-виде. */
    void selectionChanged();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void initViewer();

    Handle(V3d_Viewer)            m_viewer;
    Handle(V3d_View)              m_view;
    Handle(AIS_InteractiveContext) m_context;

    enum class CurAction {
        Nothing,
        Rotation,
        Pan,
        Zoom
    };

    CurAction m_currentAction = CurAction::Nothing;
    QPoint    m_clickPos;
    bool      m_initialized = false;
};

#endif // OCCVIEW_H
