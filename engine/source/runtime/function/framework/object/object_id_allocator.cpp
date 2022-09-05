#include "runtime/function/framework/object/object_id_allocator.h"

#include "core/base/macro.h"

namespace Pilot
{
    std::atomic<GObjectID> ObjectIDAllocator::m_next_id {0};

    GObjectID ObjectIDAllocator::alloc()
    {
        // 加载下一个 GO
        std::atomic<GObjectID> new_object_ret = m_next_id.load();

        // 加载之后，对 m_next_id 加一，方便下一次加载
        m_next_id++;

        // 如果 ID 超出了非法 ID，说明 GO 的数量已经溢出
        if (m_next_id >= k_invalid_gobject_id)
        {
            LOG_FATAL("gobject id overflow");
        }

        // 返回加载出的下一个 GO 的 ID
        return new_object_ret;
    }

} // namespace Pilot
