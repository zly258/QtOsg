#ifndef FILE_LOADING_SETTINGS_DIALOG_H
#define FILE_LOADING_SETTINGS_DIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QSettings>

/**
 * @brief 文件加载设置对话框
 * 
 * 简化版的文件加载设置，只保留必要的选项：
 * - 坐标系统选择（Y-Up, Z-Up）
 * - 单位设置（毫米、厘米、米、英寸、英尺）
 * - 自动应用设置，不显示对话框
 */
class FileSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 坐标系统类型
     */
    enum CoordinateSystem
    {
        Y_UP = 0,    // Y轴向上（OSG默认）
        Z_UP = 1     // Z轴向上（常见于某些建模软件）
    };

    /**
     * @brief 单位类型
     */
    enum UnitType
    {
        Millimeters = 0,  // 毫米
        Centimeters = 1,  // 厘米
        Meters = 2,       // 米（OSG默认）
        Inches = 3,       // 英寸
        Feet = 4          // 英尺
    };

    explicit FileSettingsDialog(QWidget *parent = nullptr);
    ~FileSettingsDialog();

    /**
     * @brief 获取坐标系统设置
     * @return 坐标系统类型
     */
    CoordinateSystem getCoordinateSystem() const;

    /**
     * @brief 获取单位类型
     * @return 单位类型
     */
    UnitType getUnitType() const;

    /**
     * @brief 获取缩放因子（基于单位转换）
     * @return 缩放因子
     */
    double getScaleFactor() const;

    /**
     * @brief 保存设置到QSettings
     */
    void saveSettings();

    /**
     * @brief 从QSettings加载设置
     */
    void loadSettings();

    /**
     * @brief 应用默认设置
     */
    void setDefaults();

    /**
     * @brief 获取静态设置（不显示对话框）
     * @return 是否成功获取设置
     */
    static bool getStaticSettings(CoordinateSystem &coordSystem, UnitType &unitType, double &scaleFactor);

signals:
    /**
     * @brief 设置改变信号
     */
    void settingsChanged();

protected:
    void accept() override;

private:
    void setupUI();
    void updateScaleFactor();
    double getUnitScaleFactor(UnitType unit) const;

private:
    // UI组件
    QComboBox* coordinateSystemCombo;
    QComboBox* unitCombo;
    QLabel* scaleFactorLabel;

    // 当前设置
    CoordinateSystem currentCoordSystem;
    UnitType currentUnitType;
};

#endif // FILE_LOADING_SETTINGS_DIALOG_H