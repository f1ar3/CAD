#ifndef EXTRUDEDIALOG_H
#define EXTRUDEDIALOG_H

#include <QDialog>

class QDoubleSpinBox;
class QCheckBox;

class ExtrudeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExtrudeDialog(QWidget* parent = nullptr);

    double height() const;
    bool symmetric() const;

    static bool getParameters(double& height, bool& symmetric, QWidget* parent);

private:
    QDoubleSpinBox* m_heightSpin = nullptr;
    QCheckBox* m_symmetricCheck = nullptr;
};

#endif // EXTRUDEDIALOG_H
