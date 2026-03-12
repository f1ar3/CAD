#include "ExtrudeDialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>

ExtrudeDialog::ExtrudeDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Выдавливание"));

    auto* layout = new QVBoxLayout(this);

    auto* hint = new QLabel(tr("Задайте высоту выдавливания эскиза в тело."), this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto* form = new QFormLayout();

    m_heightSpin = new QDoubleSpinBox(this);
    m_heightSpin->setRange(0.1, 100000.0);
    m_heightSpin->setValue(50.0);
    m_heightSpin->setDecimals(2);
    m_heightSpin->setSuffix(tr(" мм"));
    form->addRow(tr("Высота:"), m_heightSpin);

    m_symmetricCheck = new QCheckBox(tr("Симметрично (в обе стороны)"), this);
    m_symmetricCheck->setChecked(true);
    form->addRow("", m_symmetricCheck);

    layout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

double ExtrudeDialog::height() const
{
    return m_heightSpin ? m_heightSpin->value() : 50.0;
}

bool ExtrudeDialog::symmetric() const
{
    return m_symmetricCheck ? m_symmetricCheck->isChecked() : true;
}

bool ExtrudeDialog::getParameters(double& height, bool& symmetric, QWidget* parent)
{
    ExtrudeDialog dlg(parent);
    if (dlg.exec() == QDialog::Accepted) {
        height = dlg.height();
        symmetric = dlg.symmetric();
        return true;
    }
    return false;
}
