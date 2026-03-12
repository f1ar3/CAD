#include "SketchDialog.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>

SketchDialog::SketchDialog(const QString& type, QWidget* parent)
    : QDialog(parent)
{
    QString typeName;
    if (type == "Sketch_Rectangle")      typeName = tr("Прямоугольник");
    else if (type == "Sketch_Circle")    typeName = tr("Окружность");
    else if (type == "Sketch_Ellipse")   typeName = tr("Эллипс");
    else if (type == "Sketch_Polygon")   typeName = tr("Многоугольник");
    else if (type == "Sketch_Triangle")  typeName = tr("Треугольник");
    else typeName = type;

    setWindowTitle(tr("Эскиз: %1").arg(typeName));

    auto* mainLayout = new QVBoxLayout(this);

    // --- Плоскость ---
    auto* planeGroup = new QGroupBox(tr("Плоскость построения"), this);
    auto* planeLayout = new QFormLayout(planeGroup);

    m_planeCombo = new QComboBox(this);
    m_planeCombo->addItem(tr("XY (горизонтальная)"));
    m_planeCombo->addItem(tr("XZ (фронтальная)"));
    m_planeCombo->addItem(tr("YZ (боковая)"));
    planeLayout->addRow(tr("Плоскость:"), m_planeCombo);

    m_offsetSpin = new QDoubleSpinBox(this);
    m_offsetSpin->setRange(-100000.0, 100000.0);
    m_offsetSpin->setValue(0.0);
    m_offsetSpin->setDecimals(2);
    m_offsetSpin->setSuffix(tr(" мм"));
    planeLayout->addRow(tr("Смещение:"), m_offsetSpin);

    mainLayout->addWidget(planeGroup);

    // --- Параметры ---
    auto* paramGroup = new QGroupBox(tr("Параметры"), this);
    m_layout = new QFormLayout(paramGroup);
    setupFields(type);
    mainLayout->addWidget(paramGroup);

    // --- Кнопки ---
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

void SketchDialog::setupFields(const QString& type)
{
    if (type == "Sketch_Rectangle") {
        addField("Width", tr("Ширина"), 100.0);
        addField("Height", tr("Высота"), 60.0);
    } else if (type == "Sketch_Circle") {
        addField("Radius", tr("Радиус"), 50.0);
    } else if (type == "Sketch_Ellipse") {
        addField("Major Radius", tr("Большая полуось"), 60.0);
        addField("Minor Radius", tr("Малая полуось"), 30.0);
    } else if (type == "Sketch_Polygon") {
        addField("Radius", tr("Радиус вписанной окр."), 40.0);
        addField("Sides", tr("Количество сторон"), 6.0, 3.0, 64.0);
    } else if (type == "Sketch_Triangle") {
        addField("Side", tr("Сторона"), 80.0);
    }
}

void SketchDialog::addField(const QString& key, const QString& label,
                             double defaultValue, double minVal, double maxVal)
{
    auto* spin = new QDoubleSpinBox(this);
    spin->setRange(minVal, maxVal);
    spin->setValue(defaultValue);
    spin->setDecimals(2);
    if (key == "Sides")
        spin->setDecimals(0);
    else
        spin->setSuffix(tr(" мм"));
    m_layout->addRow(label + ":", spin);
    m_fields[key] = spin;
}

QMap<QString, double> SketchDialog::parameters() const
{
    QMap<QString, double> params;
    for (auto it = m_fields.constBegin(); it != m_fields.constEnd(); ++it) {
        params[it.key()] = it.value()->value();
    }
    return params;
}

int SketchDialog::planeIndex() const
{
    return m_planeCombo ? m_planeCombo->currentIndex() : 0;
}

double SketchDialog::planeOffset() const
{
    return m_offsetSpin ? m_offsetSpin->value() : 0.0;
}

QMap<QString, double> SketchDialog::getParameters(const QString& type,
                                                    int& planeIdx, double& planeOfs,
                                                    QWidget* parent)
{
    SketchDialog dlg(type, parent);
    if (dlg.exec() == QDialog::Accepted) {
        planeIdx = dlg.planeIndex();
        planeOfs = dlg.planeOffset();
        return dlg.parameters();
    }
    return {};
}
