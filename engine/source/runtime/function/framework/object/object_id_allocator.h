#pragma once

#include <atomic>
#include <limits>

namespace Pilot
{
    using GObjectID = std::size_t;      // GO 对象 ID 的类型

    constexpr GObjectID k_invalid_gobject_id = std::numeric_limits<std::size_t>::max();     // 非法 GO 对象的 ID

    class ObjectIDAllocator
    {
    public:
        static GObjectID alloc();   // 加载 ID 后，分配下一个 ID

    private:
        static std::atomic<GObjectID> m_next_id;    // 下一个 GO 对象的 ID，是原子类型
    };
} // namespace Pilot
