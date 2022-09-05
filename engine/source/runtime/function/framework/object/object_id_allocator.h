#pragma once

#include <atomic>
#include <limits>

namespace Pilot
{
    using GObjectID = std::size_t;      // GO ���� ID ������

    constexpr GObjectID k_invalid_gobject_id = std::numeric_limits<std::size_t>::max();     // �Ƿ� GO ����� ID

    class ObjectIDAllocator
    {
    public:
        static GObjectID alloc();   // ���� ID �󣬷�����һ�� ID

    private:
        static std::atomic<GObjectID> m_next_id;    // ��һ�� GO ����� ID����ԭ������
    };
} // namespace Pilot
