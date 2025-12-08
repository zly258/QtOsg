// StringUtil.h
#pragma once
#include <string>
#include <QString>

class StringUtil
{
public:
    // wstring ↔ string (UTF-8)
    static std::string WStringToString(const std::wstring &wstr);
    static std::wstring StringToWString(const std::string &str);

    // QString ↔ string (UTF-8)
    static std::string QStringToString(const QString &qstr);
    static QString StringToQString(const std::string &str);

    // 专门用于文件路径的转换（Windows需要特殊处理）
    static std::string QStringToLocalPath(const QString &qstr);
};