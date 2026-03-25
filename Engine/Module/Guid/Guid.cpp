#include "Guid.h"
#include <objbase.h>
#include <iostream>
#include <sstream>
#include <iomanip>

Guid::Guid()
{
    GUID guid;

    if (const HRESULT result = CoCreateGuid(&guid); !SUCCEEDED(result))
    {
        std::cerr << "on failed generateGUID。" << '\n';
    }

    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0')
        << std::setw(8) << guid.Data1 << "-"
        << std::setw(4) << guid.Data2 << "-"
        << std::setw(4) << guid.Data3 << "-"
        << std::setw(2) << static_cast<int>(guid.Data4[0])
        << std::setw(2) << static_cast<int>(guid.Data4[1]) << "-"
        << std::setw(2) << static_cast<int>(guid.Data4[2])
        << std::setw(2) << static_cast<int>(guid.Data4[3])
        << std::setw(2) << static_cast<int>(guid.Data4[4])
        << std::setw(2) << static_cast<int>(guid.Data4[5])
        << std::setw(2) << static_cast<int>(guid.Data4[6])
        << std::setw(2) << static_cast<int>(guid.Data4[7]);

    value_ = oss.str();
}

Guid::Guid(const std::string& guidValue)
    : value_(guidValue)
{
}

Guid Guid::Clone() const
{
    return Guid(value_);
}
