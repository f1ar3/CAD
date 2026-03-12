#ifndef SKETCHDIALOG_H
#define SKETCHDIALOG_H

#include <QDialog>
#include <QMap>
#include <QString>

class QFormLayout;
class QDoubleSpinBox;
class QSpinBox;
class QComboBox;

class SketchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SketchDialog(const QString& type, QWidget* parent = nullptr);

    QMap<QString, double> parameters() const;
    int planeIndex() const;   // 0=XY, 1=XZ, 2=YZ
    double planeOffset() const;

    static QMap<QString, double> getParameters(const QString& type,
                                                int& planeIndex, double& planeOffset,
                                                QWidget* parent);

private:
    void setupFields(const QString& type);
    void addField(const QString& key, const QString& label,
                  double defaultValue, double minVal = 0.1, double maxVal = 10000.0);

    QFormLayout* m_layout;
    QMap<QString, QDoubleSpinBox*> m_fields;
    QComboBox* m_planeCombo = nullptr;
    QDoubleSpinBox* m_offsetSpin = nullptr;
};

#endif // SKETCHDIALOG_H
