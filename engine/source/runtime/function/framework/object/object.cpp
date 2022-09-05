#include "runtime/function/framework/object/object.h"

#include "runtime/engine.h"

#include "runtime/core/meta/reflection/reflection.h"

#include "runtime/resource/asset_manager/asset_manager.h"

#include "runtime/function/framework/component/component.h"
#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/global/global_context.h"

#include <cassert>
#include <unordered_set>

#include "_generated/serializer/all_serializer.h"

namespace Pilot
{
    bool shouldComponentTick(std::string component_type_name)
    {
        if (g_is_editor_mode)
        {
            return g_editor_tick_component_types.find(component_type_name) != g_editor_tick_component_types.end();
        }
        else
        {
            return true;
        }
    }

    GObject::~GObject()
    {
        for (auto& component : m_components)
        {
            PILOT_REFLECTION_DELETE(component);
        }
        m_components.clear();
    }

    void GObject::tick(float delta_time)
    {
        for (auto& component : m_components)
        {
            // 根据运行模式确定是否需要使用 component 更新

            if (shouldComponentTick(component.getTypeName()))
            {
                // 每个 component 的 tick 函数都不一样，是 GO 的子类对象 override 的虚函数

                component->tick(delta_time);
            }
        }
    }

    bool GObject::hasComponent(const std::string& compenent_type_name) const
    {
        for (const auto& component : m_components)
        {
            if (component.getTypeName() == compenent_type_name)
                return true;
        }

        return false;
    }

    bool GObject::load(const ObjectInstanceRes& object_instance_res)
    {
        // clear old components
        // 清除 GO 对象的运算组件
        m_components.clear();

        // 设置 GO 实例资源的名称
        setName(object_instance_res.m_name);

        // load object instanced components
        // 根据 GO 实例资源来设置运算组件
        m_components = object_instance_res.m_instanced_components;

        // 对每个运算组件，加载各组件需要的资源
        for (auto component : m_components)
        {
            if (component)
            {
                // 传的不是 this 指针，而是一个 this 类似的 weak_ptr
                component->postLoadResource(weak_from_this());
            }
        }

        // load object definition components
        // 获取对象实例资源的 url / 定义
        m_definition_url = object_instance_res.m_definition;

        // 根据对象实例资源的 url，加载组件资源
        ObjectDefinitionRes definition_res;
        const bool is_loaded_success = g_runtime_global_context.m_asset_manager->loadAsset(m_definition_url, definition_res);
        if (!is_loaded_success)
            return false;

        // 对加载的每个组件
        for (auto loaded_component : definition_res.m_components)
        {
            const std::string type_name = loaded_component.getTypeName();

            // don't create component if it has been instanced
            // 如果该组件运算对象已经实例化过了，就不需要再进行实例化
            if (hasComponent(type_name))
                continue;

            // 每个组件进行 postLoadResource 操作
            loaded_component->postLoadResource(weak_from_this());

            // 在 GO 中加入加载过的组件
            m_components.push_back(loaded_component);
        }

        return true;
    }

    void GObject::save(ObjectInstanceRes& out_object_instance_res)
    {
        out_object_instance_res.m_name       = m_name;
        out_object_instance_res.m_definition = m_definition_url;

        out_object_instance_res.m_instanced_components = m_components;
    }

} // namespace Pilot