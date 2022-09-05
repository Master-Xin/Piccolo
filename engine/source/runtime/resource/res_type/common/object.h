#pragma once
#include "runtime/core/meta/reflection/reflection.h"


#include <string>
#include <vector>

namespace Pilot
{
    class Component;

    REFLECTION_TYPE(ComponentDefinitionRes)
    CLASS(ComponentDefinitionRes, Fields)
    {
        REFLECTION_BODY(ComponentDefinitionRes);

    public:
        std::string m_type_name;
        std::string m_component;
    };

    REFLECTION_TYPE(ObjectDefinitionRes)
    CLASS(ObjectDefinitionRes, Fields)
    {
        REFLECTION_BODY(ObjectDefinitionRes);

    public:
        std::vector<Reflection::ReflectionPtr<Component>> m_components;
    };

    REFLECTION_TYPE(ObjectInstanceRes)
    CLASS(ObjectInstanceRes, Fields)
    {
        REFLECTION_BODY(ObjectInstanceRes);

    public:
        std::string              m_name;            // 对象实例化资源的名称
        std::string              m_definition;      // 对象实例化资源的定义/url

        std::vector<Reflection::ReflectionPtr<Component>> m_instanced_components;   // 实例化的运算组件
    };
} // namespace Pilot
