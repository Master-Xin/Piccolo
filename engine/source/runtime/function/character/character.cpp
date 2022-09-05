#include "runtime/function/character/character.h"

#include "runtime/engine.h"
#include "runtime/function/framework/component/motor/motor_component.h"
#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/input/input_system.h"

//=========================================
#include "runtime/function/framework/component/camera/camera_component.h"
//=========================================

namespace Pilot
{
    Character::Character(std::shared_ptr<GObject> character_object) { setObject(character_object); }

    GObjectID Character::getObjectID() const
    {
        if (m_character_object)
        {
            return m_character_object->getID();
        }

        return k_invalid_gobject_id;
    }

    void Character::setObject(std::shared_ptr<GObject> gobject)
    {
        m_character_object = gobject;
        if (m_character_object)
        {
            const TransformComponent* transform_component =
                m_character_object->tryGetComponentConst(TransformComponent);
            const Transform& transform = transform_component->getTransformConst();
            m_position                 = transform.m_position;
            m_rotation                 = transform.m_rotation;
        }
        else
        {
            m_position = Vector3::ZERO;
            m_rotation = Quaternion::IDENTITY;
        }
    }

    void Character::tick(float delta_time)
    {
        if (m_character_object == nullptr)
            return;

        // 获得角色的变换计算组件
        TransformComponent* transform_component = m_character_object->tryGetComponent(TransformComponent);

        //=================================
        CameraComponent* camera_component = m_character_object->tryGetComponent(CameraComponent);
        //=================================

        // 如果 Character 修改过这个组件，就把 transform_component 中的数据也同样修改
        if (m_rotation_dirty)
        {
            transform_component->setRotation(m_rotation_buffer);
            m_rotation_dirty = false;
        }

        const MotorComponent* motor_component = m_character_object->tryGetComponentConst(MotorComponent);
        if (motor_component == nullptr)
        {
            return;
        }

        if (motor_component->getIsMoving())
        {
            m_rotation_buffer = m_rotation;

            //===================== 自己添加的 =================================
                
            Vector3 move_direction = motor_component->getMoveDirection();
            Vector3 camera_forward_direction = camera_component->getCameraForwardDirection();
            camera_forward_direction.z       = 0.f;         // 忽略 z 轴的方向信息
            camera_forward_direction.normalise();           // 忽略 z 轴信息后，需要归一化

            Radian angle_between_move_and_camera = move_direction.angleBetween(camera_forward_direction);
            // 根据夹角和旋转轴，得到四元数
            if (move_direction.crossProduct(camera_forward_direction).z >= 0)
            {
                angle_between_move_and_camera.setValue(0.f - angle_between_move_and_camera.valueRadians());
            }

            Quaternion rotate {angle_between_move_and_camera, Vector3::UNIT_Z};

            m_rotation_buffer = rotate * m_rotation_buffer;

            //====================================================================
            

            transform_component->setRotation(m_rotation_buffer);
            m_rotation_dirty = true;
        }

        const Vector3& new_position = motor_component->getTargetPosition();

        m_position = new_position;

        //float blend_ratio = std::max(1.f, motor_component->getSpeedRatio());

        //float frame_length = delta_time * blend_ratio;
        //m_position =
        //    (m_position * (k_camera_blend_time - frame_length) + new_position * frame_length) / k_camera_blend_time;
        //m_position =
        //    (m_position * (k_camera_blend_time - frame_length) + new_position * frame_length) / k_camera_blend_time;
    }
} // namespace Pilot