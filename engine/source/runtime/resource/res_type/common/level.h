#pragma once
#include "runtime/core/meta/reflection/reflection.h"

#include "runtime/resource/res_type/common/object.h"

namespace Pilot
{
    REFLECTION_TYPE(LevelRes)
    CLASS(LevelRes, Fields)
    {
        REFLECTION_BODY(LevelRes);

    public:
        Vector3 m_gravity {0.f, 0.f, -9.8f};        // 重力的方向、大小
        std::string m_character_name;               // 角色名称

        std::vector<ObjectInstanceRes> m_objects;   // 对象实例资源的容器
    };
} // namespace Pilot