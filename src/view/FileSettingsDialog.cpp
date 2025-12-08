#include "view/FileSettingsDialog.h"
#include <QSettings>
#include <QFont>
#include <QPushButton>

FileSettingsDialog::FileSettingsDialog(QWidget *parent)
    : QDialog(parent),
      coordinateSystemCombo(nullptr),
      unitCombo(nullptr),
      scaleFactorLabel(nullptr),
      currentCoordSystem(Y_UP),
      currentUnitType(Meters)
{
    setWindowTitle("文件加载设置");
    setMinimumSize(300, 200);
    setupUI();
    loadSettings();
}

FileSettingsDialog::~FileSettingsDialog()
{
}

void FileSettingsDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 坐标系统设置
    QGroupBox* coordGroup = new QGroupBox("坐标系统");
    QVBoxLayout* coordLayout = new QVBoxLayout(coordGroup);
    
    coordinateSystemCombo = new QComboBox();
    coordinateSystemCombo->addItem("Y轴向上 (Y-Up)", Y_UP);
    coordinateSystemCombo->addItem("Z轴向上 (Z-Up)", Z_UP);
    
    coordLayout->addWidget(new QLabel("坐标系统:"));
    coordLayout->addWidget(coordinateSystemCombo);
    mainLayout->addWidget(coordGroup);
    
    // 单位设置
    QGroupBox* unitGroup = new QGroupBox("模型单位");
    QVBoxLayout* unitLayout = new QVBoxLayout(unitGroup);
    
    unitCombo = new QComboBox();
    unitCombo->addItem("毫米 (mm)", Millimeters);
    unitCombo->addItem("厘米 (cm)", Centimeters);
    unitCombo->addItem("米 (m) - OSG默认", Meters);
    unitCombo->addItem("英寸 (inch)", Inches);
    unitCombo->addItem("英尺 (ft)", Feet);
    
    unitLayout->addWidget(new QLabel("导入模型单位:"));
    unitLayout->addWidget(unitCombo);
    
    // 缩放因子显示
    QHBoxLayout* scaleLayout = new QHBoxLayout();
    scaleLayout->addWidget(new QLabel("缩放因子:"));
    scaleFactorLabel = new QLabel("1.000");
    scaleLayout->addWidget(scaleFactorLabel);
    scaleLayout->addStretch();
    unitLayout->addLayout(scaleLayout);
    
    mainLayout->addWidget(unitGroup);
    
    mainLayout->addStretch();

    // 按钮
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    // 添加重置按钮
    QPushButton* resetButton = buttonBox->button(QDialogButtonBox::Reset);
    connect(resetButton, &QPushButton::clicked, this, &FileSettingsDialog::setDefaults);
    
    mainLayout->addWidget(buttonBox);
    
    // 连接信号
    connect(coordinateSystemCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this]() { updateScaleFactor(); });
    connect(unitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this]() { updateScaleFactor(); });
    
    updateScaleFactor(); // 初始化显示
}

void FileSettingsDialog::updateScaleFactor()
{
    double scale = getScaleFactor();
    scaleFactorLabel->setText(QString("%1").arg(scale, 0, 'f', 3));
}

FileSettingsDialog::CoordinateSystem FileSettingsDialog::getCoordinateSystem() const
{
    int index = coordinateSystemCombo->currentIndex();
    return static_cast<CoordinateSystem>(coordinateSystemCombo->itemData(index).toInt());
}

FileSettingsDialog::UnitType FileSettingsDialog::getUnitType() const
{
    int index = unitCombo->currentIndex();
    return static_cast<UnitType>(unitCombo->itemData(index).toInt());
}

double FileSettingsDialog::getScaleFactor() const
{
    UnitType unit = getUnitType();
    return getUnitScaleFactor(unit);
}

double FileSettingsDialog::getUnitScaleFactor(UnitType unit) const
{
    switch (unit) {
        case Millimeters: return 0.001;    // 毫米 -> 米
        case Centimeters: return 0.01;     // 厘米 -> 米
        case Meters:      return 1.0;      // 米 -> 米（OSG默认）
        case Inches:      return 0.0254;   // 英寸 -> 米
        case Feet:        return 0.3048;   // 英尺 -> 米
        default:          return 1.0;
    }
}

void FileSettingsDialog::saveSettings()
{
    QSettings settings("LMBModelViewer", "LMBModelViewer");
    settings.setValue("FileLoading/CoordinateSystem", static_cast<int>(getCoordinateSystem()));
    settings.setValue("FileLoading/UnitType", static_cast<int>(getUnitType()));
}

void FileSettingsDialog::loadSettings()
{
    QSettings settings("LMBModelViewer", "LMBModelViewer");
    
    CoordinateSystem coord = static_cast<CoordinateSystem>(
        settings.value("FileLoading/CoordinateSystem", Y_UP).toInt());
    UnitType unit = static_cast<UnitType>(
        settings.value("FileLoading/UnitType", Meters).toInt());
    
    coordinateSystemCombo->setCurrentIndex(coord);
    unitCombo->setCurrentIndex(unit);
    
    currentCoordSystem = coord;
    currentUnitType = unit;
    
    updateScaleFactor();
}

void FileSettingsDialog::setDefaults()
{
    coordinateSystemCombo->setCurrentIndex(Y_UP);
    unitCombo->setCurrentIndex(Meters);
    updateScaleFactor();
}

bool FileSettingsDialog::getStaticSettings(CoordinateSystem &coordSystem, UnitType &unitType, double &scaleFactor)
{
    QSettings settings("LMBModelViewer", "LMBModelViewer");
    
    coordSystem = static_cast<CoordinateSystem>(
        settings.value("FileLoading/CoordinateSystem", Y_UP).toInt());
    unitType = static_cast<UnitType>(
        settings.value("FileLoading/UnitType", Meters).toInt());
    
    FileSettingsDialog dialog;
    scaleFactor = dialog.getUnitScaleFactor(unitType);
    
    return true;
}

void FileSettingsDialog::accept()
{
    saveSettings();
    QDialog::accept();
}