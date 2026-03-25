#pragma once
class Guid;

struct GuidHash final
{
    size_t operator()(const Guid& guid) const;
};
