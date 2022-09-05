#include "runtime/function/framework/object/object_id_allocator.h"

#include "core/base/macro.h"

namespace Pilot
{
    std::atomic<GObjectID> ObjectIDAllocator::m_next_id {0};

    GObjectID ObjectIDAllocator::alloc()
    {
        // ������һ�� GO
        std::atomic<GObjectID> new_object_ret = m_next_id.load();

        // ����֮�󣬶� m_next_id ��һ��������һ�μ���
        m_next_id++;

        // ��� ID �����˷Ƿ� ID��˵�� GO �������Ѿ����
        if (m_next_id >= k_invalid_gobject_id)
        {
            LOG_FATAL("gobject id overflow");
        }

        // ���ؼ��س�����һ�� GO �� ID
        return new_object_ret;
    }

} // namespace Pilot
