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
        Vector3 m_gravity {0.f, 0.f, -9.8f};        // �����ķ��򡢴�С
        std::string m_character_name;               // ��ɫ����

        std::vector<ObjectInstanceRes> m_objects;   // ����ʵ����Դ������
    };
} // namespace Pilot