#include "PropertyPanel.h"
#include "../model/Document.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QColorDialog>
#include <QPixmap>
#include <QSlider>
#include <QComboBox>
#include <QHBoxLayout>

// Параметры русских подписей для spin-боксов
static QMap<QString, QString> paramLabels()
{
    return {
        {"Width (X)",      QObject::tr("Ширина (X)")},
        {"Depth (Y)",      QObject::tr("Глубина (Y)")},
        {"Height (Z)",     QObject::tr("Высота (Z)")},
        {"Radius",         QObject::tr("Радиус")},
        {"Height",         QObject::tr("Высота")},
        {"Bottom Radius",  QObject::tr("Нижний радиус")},
        {"Top Radius",     QObject::tr("Верхний радиус")},
        {"Major Radius",   QObject::tr("Большой радиус")},
        {"Minor Radius",   QObject::tr("Малый радиус")},
        {"Top Width (Xmin)", QObject::tr("Верхняя ширина")},
        {"radius",         QObject::tr("Радиус")},
        {"distance",       QObject::tr("Расстояние")},
    };
}

QStringList PropertyPanel::editableTypes()
{
    return {"Box", "Cylinder", "Sphere", "Cone", "Torus", "Wedge"};
}

PropertyPanel::PropertyPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);

    // --- Информация ---
    auto* infoGroup = new QGroupBox(tr("Информация"), this);
    auto* infoLayout = new QFormLayout(infoGroup);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("Имя фигуры"));
    connect(m_nameEdit, &QLineEdit::editingFinished, this, &PropertyPanel::onNameChanged);
    infoLayout->addRow(tr("Имя:"), m_nameEdit);

    m_typeLabel = new QLabel("-", this);
    infoLayout->addRow(tr("Тип:"), m_typeLabel);

    m_colorButton = new QPushButton(this);
    m_colorButton->setFixedSize(60, 24);
    connect(m_colorButton, &QPushButton::clicked, this, &PropertyPanel::onColorClicked);
    infoLayout->addRow(tr("Цвет:"), m_colorButton);

    // Transparency slider
    auto* transpWidget = new QWidget(this);
    auto* transpLayout = new QHBoxLayout(transpWidget);
    transpLayout->setContentsMargins(0, 0, 0, 0);
    m_transparencySlider = new QSlider(Qt::Horizontal, this);
    m_transparencySlider->setRange(0, 100);
    m_transparencySlider->setValue(0);
    connect(m_transparencySlider, &QSlider::valueChanged, this, &PropertyPanel::onTransparencyChanged);
    m_transparencyLabel = new QLabel("0%", this);
    m_transparencyLabel->setFixedWidth(35);
    transpLayout->addWidget(m_transparencySlider);
    transpLayout->addWidget(m_transparencyLabel);
    infoLayout->addRow(tr("Прозрачность:"), transpWidget);

    // Display mode combo
    m_displayModeCombo = new QComboBox(this);
    m_displayModeCombo->addItem(tr("Каркас"));
    m_displayModeCombo->addItem(tr("Заливка"));
    m_displayModeCombo->setCurrentIndex(1);
    connect(m_displayModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertyPanel::onDisplayModeChanged);
    infoLayout->addRow(tr("Отображение:"), m_displayModeCombo);

    mainLayout->addWidget(infoGroup);

    // --- Позиция ---
    auto* posGroup = new QGroupBox(tr("Позиция"), this);
    auto* posLayout = new QFormLayout(posGroup);

    auto makePosSpin = [this]() {
        auto* spin = new QDoubleSpinBox(this);
        spin->setRange(-100000.0, 100000.0);
        spin->setDecimals(2);
        spin->setSuffix(tr(" мм"));
        spin->setValue(0.0);
        connect(spin, &QDoubleSpinBox::editingFinished, this, &PropertyPanel::onPositionChanged);
        return spin;
    };

    m_posX = makePosSpin();
    m_posY = makePosSpin();
    m_posZ = makePosSpin();
    posLayout->addRow("X:", m_posX);
    posLayout->addRow("Y:", m_posY);
    posLayout->addRow("Z:", m_posZ);
    mainLayout->addWidget(posGroup);

    // --- Параметры ---
    auto* paramGroup = new QGroupBox(tr("Параметры"), this);
    m_formLayout = new QFormLayout(paramGroup);
    mainLayout->addWidget(paramGroup);

    mainLayout->addStretch();

    clearPanel();
}

void PropertyPanel::setDocument(Document* doc)
{
    m_document = doc;
}

void PropertyPanel::showShape(int id)
{
    m_updatingUI = true;

    clearParamFields();
    m_currentShapeId = id;

    if (!m_document) {
        clearPanel();
        m_updatingUI = false;
        return;
    }

    const ShapeEntry* entry = m_document->findShape(id);
    if (!entry) {
        clearPanel();
        m_updatingUI = false;
        return;
    }

    m_nameEdit->setText(entry->name);
    m_nameEdit->setEnabled(true);
    m_typeLabel->setText(entry->type);
    updateColorButton(entry->color);
    m_colorButton->setEnabled(true);

    int transpPercent = static_cast<int>(entry->transparency * 100);
    m_transparencySlider->setValue(transpPercent);
    m_transparencyLabel->setText(QString("%1%").arg(transpPercent));
    m_transparencySlider->setEnabled(true);
    m_displayModeCombo->setCurrentIndex(entry->displayMode);
    m_displayModeCombo->setEnabled(true);

    m_posX->setValue(entry->posX);
    m_posY->setValue(entry->posY);
    m_posZ->setValue(entry->posZ);
    m_posX->setEnabled(true);
    m_posY->setEnabled(true);
    m_posZ->setEnabled(true);

    bool editable = editableTypes().contains(entry->type);
    auto labels = paramLabels();

    for (auto it = entry->params.constBegin(); it != entry->params.constEnd(); ++it) {
        QString displayLabel = labels.value(it.key(), it.key());

        if (editable) {
            auto* spin = new QDoubleSpinBox(this);
            spin->setRange(0.1, 100000.0);
            spin->setDecimals(2);
            spin->setSuffix(tr(" мм"));
            spin->setValue(it.value());
            spin->setProperty("paramKey", it.key());
            connect(spin, &QDoubleSpinBox::editingFinished, this, &PropertyPanel::onParamChanged);
            m_formLayout->addRow(displayLabel + ":", spin);
            m_paramSpinBoxes[it.key()] = spin;
        } else {
            auto* label = new QLabel(QString::number(it.value(), 'f', 2) + tr(" мм"), this);
            m_formLayout->addRow(displayLabel + ":", label);
        }
    }

    m_updatingUI = false;
}

void PropertyPanel::clearPanel()
{
    m_currentShapeId = -1;
    m_nameEdit->clear();
    m_nameEdit->setEnabled(false);
    m_typeLabel->setText("-");
    updateColorButton(QColor(180, 180, 220));
    m_colorButton->setEnabled(false);
    m_transparencySlider->setValue(0);
    m_transparencySlider->setEnabled(false);
    m_displayModeCombo->setCurrentIndex(1);
    m_displayModeCombo->setEnabled(false);
    m_posX->setValue(0.0);
    m_posY->setValue(0.0);
    m_posZ->setValue(0.0);
    m_posX->setEnabled(false);
    m_posY->setEnabled(false);
    m_posZ->setEnabled(false);
    clearParamFields();
}

void PropertyPanel::clearParamFields()
{
    m_paramSpinBoxes.clear();
    while (m_formLayout->rowCount() > 0) {
        m_formLayout->removeRow(0);
    }
}

void PropertyPanel::onNameChanged()
{
    if (m_updatingUI || !m_document || m_currentShapeId < 0) return;
    QString newName = m_nameEdit->text().trimmed();
    if (newName.isEmpty()) return;
    m_document->renameShape(m_currentShapeId, newName);
}

void PropertyPanel::onColorClicked()
{
    if (!m_document || m_currentShapeId < 0) return;

    const ShapeEntry* entry = m_document->findShape(m_currentShapeId);
    if (!entry) return;

    QColor color = QColorDialog::getColor(entry->color, this, tr("Выберите цвет"));
    if (!color.isValid()) return;

    m_document->setShapeColor(m_currentShapeId, color);
    updateColorButton(color);
}

void PropertyPanel::onParamChanged()
{
    if (m_updatingUI || !m_document || m_currentShapeId < 0) return;

    QMap<QString, double> newParams;
    for (auto it = m_paramSpinBoxes.constBegin(); it != m_paramSpinBoxes.constEnd(); ++it) {
        newParams[it.key()] = it.value()->value();
    }

    m_document->updateShapeParams(m_currentShapeId, newParams);
}

void PropertyPanel::onTransparencyChanged(int value)
{
    if (m_updatingUI || !m_document || m_currentShapeId < 0) return;
    m_transparencyLabel->setText(QString("%1%").arg(value));
    m_document->setShapeTransparency(m_currentShapeId, value / 100.0);
}

void PropertyPanel::onDisplayModeChanged(int index)
{
    if (m_updatingUI || !m_document || m_currentShapeId < 0) return;
    m_document->setDisplayMode(m_currentShapeId, index);
}

void PropertyPanel::onPositionChanged()
{
    if (m_updatingUI || !m_document || m_currentShapeId < 0) return;
    m_document->setShapePosition(m_currentShapeId,
                                  m_posX->value(), m_posY->value(), m_posZ->value());
}

void PropertyPanel::updateColorButton(const QColor& color)
{
    QPixmap pix(48, 16);
    pix.fill(color);
    m_colorButton->setIcon(QIcon(pix));
    m_colorButton->setText(color.name());
}
