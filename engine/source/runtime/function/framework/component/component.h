#pragma once
#include "runtime/core/meta/reflection/reflection.h"

namespace Pilot
{
    class GObject;

    // Component�������� GO ���������
    REFLECTION_TYPE(Component)
    CLASS(Component, WhiteListFields)
    {
        REFLECTION_BODY(Component)
    protected:
        std::weak_ptr<GObject> m_parent_object;     // ������
        bool     m_is_dirty {false};                // ��ִ����������ʱ�򣬻����� dirty ��־��ͬʱֻ������һ���������

    public:
        Component() = default;
        virtual ~Component() {}

        // Instantiating the component after definition loaded
        // ÿ�� Component �� postLoadResource �������ݲ�һ��
        virtual void postLoadResource(std::weak_ptr<GObject> parent_object) { m_parent_object = parent_object;}

        // ÿ�� Component ����Ҫ�������߼�
        virtual void tick(float delta_time) {};

        // �鿴�Ƿ���ĳ�����������ִ��
        bool isDirty() const { return m_is_dirty; }

        // ���������������ı�־
        void setDirtyFlag(bool is_dirty) { m_is_dirty = is_dirty; }

        // ��������У��༭����ѭ������Ĭ���ǹرյ�
        bool m_tick_in_editor_mode {false};
    };

} // namespace Pilot
