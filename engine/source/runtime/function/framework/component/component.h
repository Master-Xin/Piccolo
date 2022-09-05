#pragma once
#include "runtime/core/meta/reflection/reflection.h"

namespace Pilot
{
    class GObject;

    // Component，是所有 GO 的运算组件
    REFLECTION_TYPE(Component)
    CLASS(Component, WhiteListFields)
    {
        REFLECTION_BODY(Component)
    protected:
        std::weak_ptr<GObject> m_parent_object;     // 父对象
        bool     m_is_dirty {false};                // 在执行组件运算的时候，会设置 dirty 标志，同时只能运行一种组件运算

    public:
        Component() = default;
        virtual ~Component() {}

        // Instantiating the component after definition loaded
        // 每个 Component 的 postLoadResource 函数内容不一样
        virtual void postLoadResource(std::weak_ptr<GObject> parent_object) { m_parent_object = parent_object;}

        // 每个 Component 的主要的运算逻辑
        virtual void tick(float delta_time) {};

        // 查看是否有某个组件运算在执行
        bool isDirty() const { return m_is_dirty; }

        // 设置组件正在运算的标志
        void setDirtyFlag(bool is_dirty) { m_is_dirty = is_dirty; }

        // 在组件类中，编辑器的循环函数默认是关闭的
        bool m_tick_in_editor_mode {false};
    };

} // namespace Pilot
