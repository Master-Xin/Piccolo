#include "runtime/function/framework/component/motor/motor_component.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/character/character.h"
#include "runtime/function/controller/character_controller.h"
#include "runtime/function/framework/component/animation/animation_component.h"
#include "runtime/function/framework/component/camera/camera_component.h"
#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/level/level.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/input/input_system.h"
#include "runtime/function/physics/physics_scene.h"

namespace Pilot
{
    void MotorComponent::postLoadResource(std::weak_ptr<GObject> parent_object)
    {
        // ����������󶼻���ת�� phisics actor��Ӧ�����д���
        m_parent_object = parent_object;


        if (m_motor_res.m_controller_config.getTypeName() == "PhysicsControllerConfig")
        {
            m_controller_type = ControllerType::physics;
            PhysicsControllerConfig* controller_config =
                static_cast<PhysicsControllerConfig*>(m_motor_res.m_controller_config);
            m_controller = new CharacterController(controller_config->m_capsule_shape);
        }
        else if (m_motor_res.m_controller_config != nullptr)
        {
            m_controller_type = ControllerType::invalid;
            LOG_ERROR("invalid controller type, not able to move");
        }

        const TransformComponent* transform_component = parent_object.lock()->tryGetComponentConst(TransformComponent);

        m_target_position = transform_component->getPosition();
    }

    MotorComponent::~MotorComponent()
    {
        if (m_controller_type == ControllerType::physics)
        {
            delete m_controller;
            m_controller = nullptr;
        }
    }

    void MotorComponent::tick(float delta_time)
    {
        tickPlayerMotor(delta_time);
    }

    void MotorComponent::tickPlayerMotor(float delta_time)
    {
        // ��ȡ������������ɹ��Ϳ��Լ���ִ�С��Ᵽ֤���̰߳�ȫ
        if (!m_parent_object.lock())
            return;

        // ��ȡ�洢�� GO ����� level ��������еĻ�Ծ�� character
        std::shared_ptr<Level>     current_level     = g_runtime_global_context.m_world_manager->getCurrentActiveLevel().lock();
        std::shared_ptr<Character> current_character = current_level->getCurrentActiveCharacter().lock();
        if (current_character == nullptr)
            return;

        // ��ȡ��ǰ��Ծ��ɫ�� ID
        if (current_character->getObjectID() != m_parent_object.lock()->getID())
            return;

        // ��ȡ transform �������������
        TransformComponent* transform_component =
            m_parent_object.lock()->tryGetComponent<TransformComponent>("TransformComponent");

        //============================================================
        CameraComponent* camera_component =
            m_parent_object.lock()->tryGetComponent<CameraComponent>("CameraComponent");
        //============================================================

        // ���ݹ������ӽǵ�ƫת��
        Radian turn_angle_yaw = g_runtime_global_context.m_input_system->m_cursor_delta_yaw;

        // ��������ָ������
        unsigned int command = g_runtime_global_context.m_input_system->getGameCommand();

        if (command >= (unsigned int)GameCommand::invalid)
            return;

        // ���� ����ָ�delta_time������������ˮƽ�ƶ��ٶ�
        calculatedDesiredHorizontalMoveSpeed(command, delta_time);

        // ���� ���ָ�delta_time�����������Ĵ�ֱ�ƶ��ٶ�
        calculatedDesiredVerticalMoveSpeed(command, delta_time);

        //// ���� ���ָ���ǰ��ɫ�����������µĳ��򣬼����ƶ�����
        //calculatedDesiredMoveDirection(command, transform_component->getRotation());

        //==================== �¸Ķ��� ============================
        calculatedDesiredMoveDirectionNew(command, camera_component->getCameraForwardDirection());

        // �����߼�����λ��
        calculateDesiredDisplacement(delta_time);

        // ���������ƶ�λ��
        calculateTargetPosition(transform_component->getPosition());
        
        // 
        transform_component->setPosition(m_target_position);

        AnimationComponent* animation_component =
            m_parent_object.lock()->tryGetComponent<AnimationComponent>("AnimationComponent");
        if (animation_component != nullptr)
        {
            animation_component->updateSignal("speed", m_target_position.distance(transform_component->getPosition()) / delta_time);
            animation_component->updateSignal("jumping", m_jump_state != JumpState::idle);
        }
    }

    void MotorComponent::calculatedDesiredHorizontalMoveSpeed(unsigned int command, float delta_time)
    {
        // �����ж��Ƿ�ˮƽ�ƶ�
        bool has_move_command = ((unsigned int)GameCommand::forward | (unsigned int)GameCommand::backward |
                                 (unsigned int)GameCommand::left | (unsigned int)GameCommand::right) &
                                command;
        // �����ж��Ƿ���
        bool has_sprint_command = (unsigned int)GameCommand::sprint & command;

        // �Ƿ���ٱ�־
        bool  is_acceleration    = false;

        // ���ٶȳ�ʼ��Ϊ�ĵ���������ƶ�״̬�µļ��ٶ�
        float final_acceleration = m_motor_res.m_move_acceleration;

        // ��ʼ��������С���ٶȱ���
        float min_speed_ratio    = 0.f;
        float max_speed_ratio    = 0.f;

        // ����ƶ���
        if (has_move_command)
        {
            // �򿪼��ٿ���
            is_acceleration = true;
            // ��ȡ �ܹ��������� ����ƶ��ٶȱ���
            max_speed_ratio = m_motor_res.m_max_move_speed_ratio;

            // ��� ��ǰ�ƶ��ٶȱ��� ���ڵ��� ����������ƶ��ٶȱ��ʣ���ζ�ſ���Ҫ���ٵ�������ٶȣ�������Ǽ��٣�
            if (m_move_speed_ratio >= m_motor_res.m_max_move_speed_ratio)
            {
                // ʵ�����յļ��ٶ� ��Ϊ �����ĳ�̼��ٶ�
                final_acceleration = m_motor_res.m_sprint_acceleration;
                // ��ȡʵ���Ƿ��ڳ��
                is_acceleration    = has_sprint_command;
                // �ƶ��ٶ��Ѿ������ܹ�������������ٶȣ����˴��ٶȵı仯��Χ����Сֵ��Ϊ����ܹ�������������ƶ��ٶ�
                min_speed_ratio    = m_motor_res.m_max_move_speed_ratio;
                // �˴��ٶȵı仯��Χ�����ֵ��Ϊ����ܹ���������������ٶ�
                max_speed_ratio    = m_motor_res.m_max_sprint_speed_ratio;
            }
        }
        // ���û���ƶ�
        else
        {
            // �������
            is_acceleration = false;
            // ��Ҫ���ٵ� 0
            min_speed_ratio = 0.f;
            // ����ٶȱ����� ����ܹ���������������ٶ�
            max_speed_ratio = m_motor_res.m_max_sprint_speed_ratio;
        }

        // ���㵱ǰ���ƶ��ٶȱ��ʣ����Ǽ��پ��Ǽ���
        m_move_speed_ratio += (is_acceleration ? 1.0f : -1.0f) * final_acceleration * delta_time;
        // ǯ��ס�ٶȱ��ʣ�Ҳ�����޷�
        m_move_speed_ratio = std::clamp(m_move_speed_ratio, min_speed_ratio, max_speed_ratio);
    }

    void MotorComponent::calculatedDesiredVerticalMoveSpeed(unsigned int command, float delta_time)
    {
        // ��ȡ��������һЩ����
        std::shared_ptr<PhysicsScene> physics_scene =
            g_runtime_global_context.m_world_manager->getCurrentActivePhysicsScene().lock();
        ASSERT(physics_scene);

        // ��� ������ ֧�ֵ���Ծ�߶��� 0��ֱ�ӷ��أ����Դ�ֱ�����ٶȽ��иı�
        if (m_motor_res.m_jump_height == 0.f)
            return;

        // ��ȡ�������������ĳ��ȣ�Ҳ���������Ĵ�С
        const float gravity = physics_scene->getGravity().length();

        // JumpState ��������״̬����ֹ���½�������

        // �����ǰ��Ծ״̬���Ǿ�ֹ������û�д���������
        if (m_jump_state == JumpState::idle && m_controller->isTouchGround() == false)
        {
            // ����Ծ״̬��Ϊ�½�
            m_jump_state = JumpState::falling;
        }

        // �����Ծ״̬�Ǿ�ֹ
        if (m_jump_state == JumpState::idle)
        {
            // ���ָ������Ծ������
            if ((unsigned int)GameCommand::jump & command)
            {
                // ��Ծ״̬�л�Ϊ����
                m_jump_state                    = JumpState::rising;
                // ��ֱ����ĳ��ٶȣ�v = ������(2gh)
                m_vertical_move_speed           = Math::sqrt(m_motor_res.m_jump_height * 2 * gravity);
                // ��Ծʱˮƽ������ƶ��ٶ�
                m_jump_horizontal_speed_ratio   = m_move_speed_ratio;
            }
            else
            {
                // ��Ծ��û�б����£���ֱ�����ٶȾ��� 0 
                m_vertical_move_speed = 0.f;
            }
        }
        // �����Ծ״̬�������������½�
        else if (m_jump_state == JumpState::rising || m_jump_state == JumpState::falling)
        {
            // ��ֱ�ƶ��ٶȸ��£���������������ٶȳ��ϣ�����Ǹ���������ٶȳ���
            m_vertical_move_speed -= gravity * delta_time;
            if (m_vertical_move_speed <= 0.f)
            {
                m_jump_state = JumpState::falling;
            }
        }
    }

    void MotorComponent::calculatedDesiredMoveDirection(unsigned int command, const Quaternion& object_rotation)
    {

        // ���������Ծ״̬
        if (m_jump_state == JumpState::idle)
        {
            // ǰ��ķ���������ģ������ϵ�µ� -y ����
            Vector3 forward_dir = object_rotation * Vector3::NEGATIVE_UNIT_Y;
            // ����ķ���������ģ������ϵ�µ� x ����
            Vector3 left_dir    = object_rotation * Vector3::UNIT_X;


            // ��ָ�����
            if (command > 0)
            {
                m_desired_horizontal_move_direction = Vector3::ZERO;
            }

            // ��ǰ
            if ((unsigned int)GameCommand::forward & command)
            {
                m_desired_horizontal_move_direction += forward_dir;
            }

            // ����
            if ((unsigned int)GameCommand::backward & command)
            {
                m_desired_horizontal_move_direction -= forward_dir;
            }
            // ����
            if ((unsigned int)GameCommand::left & command)
            {
                m_desired_horizontal_move_direction += left_dir;
            }
            // ����
            if ((unsigned int)GameCommand::right & command)
            {
                m_desired_horizontal_move_direction -= left_dir;
            }
            // ����������һ��
            m_desired_horizontal_move_direction.normalise();
        }
    }

    void MotorComponent::calculatedDesiredMoveDirectionNew(unsigned int command, const Vector3& camera_direction)
    {

        // ���������Ծ״̬
        if (m_jump_state == JumpState::idle)
        {
            // ǰ��ķ�������
            Vector3 forward_dir = camera_direction;
            forward_dir.z       = 0.f;            // ���� z ��ķ�����Ϣ
            forward_dir.normalise();              // ���� z ����Ϣ����Ҫ��һ��

            // ����ķ���������ģ������ϵ�µ� x ����
            Vector3 left_dir {0.f - forward_dir.y, forward_dir.x, forward_dir.z};

            if (command > 0)
            {
                m_desired_horizontal_move_direction = Vector3::ZERO;
            }
            if ((unsigned int)GameCommand::forward & command)
            {
                m_desired_horizontal_move_direction += forward_dir;
            }
            if ((unsigned int)GameCommand::backward & command)
            {
                m_desired_horizontal_move_direction -= forward_dir;
            }
            if ((unsigned int)GameCommand::left & command)
            {
                m_desired_horizontal_move_direction += left_dir;
            }
            if ((unsigned int)GameCommand::right & command)
            {
                m_desired_horizontal_move_direction -= left_dir;
            }
            m_desired_horizontal_move_direction.normalise();
        }
    }

    void MotorComponent::calculateDesiredDisplacement(float delta_time)
    {
        float horizontal_speed_ratio =
            m_jump_state == JumpState::idle ? m_move_speed_ratio : m_jump_horizontal_speed_ratio;
        m_desired_displacement =
            m_desired_horizontal_move_direction * m_motor_res.m_move_speed * horizontal_speed_ratio * delta_time +
            Vector3::UNIT_Z * m_vertical_move_speed * delta_time;
    }

    void MotorComponent::calculateTargetPosition(const Vector3&& current_position)
    {
        Vector3 final_position;

        switch (m_controller_type)
        {
            case ControllerType::none:
                final_position = current_position + m_desired_displacement;
                break;
            case ControllerType::physics:
                final_position = m_controller->move(current_position, m_desired_displacement);
                break;
            default:
                final_position = current_position;
                break;
        }

        if (m_jump_state == JumpState::falling && m_controller->isTouchGround())
        {
            m_jump_state = JumpState::idle;
        }

        m_is_moving       = (final_position - current_position).squaredLength() > 0.f;
        m_target_position = final_position;
    }

} // namespace Pilot
