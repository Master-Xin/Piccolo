#include "runtime/function/global/global_context.h"

#include "core/log/log_system.h"

#include "runtime/engine.h"

#include "runtime/platform/file_service/file_service.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/engine.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/input/input_system.h"
#include "runtime/function/physics/physics_system.h"
#include "runtime/function/physics/physics_manager.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/window_system.h"

namespace Pilot
{
    RuntimeGlobalContext g_runtime_global_context;

    void RuntimeGlobalContext::startSystems(const EngineInitParams& init_params)
    {
        // 配置管理器的初始化，一些相关的资产、字体目录
        m_config_manager = std::make_shared<ConfigManager>();
        m_config_manager->initialize(init_params);

        // 文件系统的初始化，主要是路径
        m_file_system = std::make_shared<FileSystem>();

        // 日志系统初始化，包括等级设置等等
        m_logger_system = std::make_shared<LogSystem>();

        // 资产管理器初始化，主要是 json 格式的转化，读取相关信息
        m_asset_manager = std::make_shared<AssetManager>();

        // 物理系统初始化，包括重力、光线投射等计算
        m_legacy_physics_system = std::make_shared<PhysicsSystem>();

        m_physics_manager = std::make_shared<PhysicsManager>();
        m_physics_manager->initialize();

        // 世界管理器初始化
        m_world_manager = std::make_shared<WorldManager>();
        m_world_manager->initialize();

        // 窗口系统初始化，包括长宽、是否全屏等信息
        m_window_system = std::make_shared<WindowSystem>();
        WindowCreateInfo window_create_info;
        m_window_system->initialize(window_create_info);

        // 输入系统初始化，包括光标、键盘等设备
        m_input_system = std::make_shared<InputSystem>();
        m_input_system->initialize();

        // 渲染系统初始化，包括一些网格数据等等
        m_render_system = std::make_shared<RenderSystem>();
        RenderSystemInitInfo render_init_info;
        render_init_info.window_system = m_window_system;
        m_render_system->initialize(render_init_info);
    }

    void RuntimeGlobalContext::shutdownSystems()
    {
        m_render_system.reset();

        m_window_system.reset();

        m_world_manager->clear();
        m_world_manager.reset();

        m_legacy_physics_system.reset();

        m_physics_manager->clear();
        m_physics_manager.reset();

        m_input_system->clear();
        m_input_system.reset();

        m_asset_manager.reset();


        m_logger_system.reset();

        m_file_system.reset();

        m_config_manager.reset();
    }
} // namespace Pilot