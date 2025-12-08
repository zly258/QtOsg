// StringUtil.cpp
#include "utils/StringUtil.h"

#ifdef _WIN32
#include <Windows.h> // 仅Windows需要
#endif

// wstring ↔ string (UTF-8编码)
std::string StringUtil::WStringToString(const std::wstring &wstr)
{
    return QString::fromStdWString(wstr).toUtf8().toStdString();
}

std::wstring StringUtil::StringToWString(const std::string &str)
{
    return QString::fromUtf8(str.c_str()).toStdWString();
}

// QString ↔ string (UTF-8编码)
std::string StringUtil::QStringToString(const QString &qstr)
{
    return qstr.toUtf8().toStdString();
}

QString StringUtil::StringToQString(const std::string &str)
{
    return QString::fromUtf8(str.c_str());
}

// 专门处理文件路径（Windows转换为本地编码）
std::string StringUtil::QStringToLocalPath(const QString &qstr)
{
#ifdef _WIN32
    // Windows: 转换为本地代码页（如GBK）
    QByteArray local = qstr.toLocal8Bit();
    return std::string(local.constData(), local.length());
#else
    // Linux/macOS: 直接使用UTF-8
    return qstr.toUtf8().toStdString();
#endif
}