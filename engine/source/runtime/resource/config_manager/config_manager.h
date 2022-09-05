#pragma once

#include <filesystem>

namespace Pilot
{
    struct EngineInitParams;

    class ConfigManager
    {
    public:
        void initialize(const EngineInitParams& init_param);

        const std::filesystem::path& getRootFolder() const;             // 根目录
        const std::filesystem::path& getAssetFolder() const;            // 资产目录
        const std::filesystem::path& getSchemaFolder() const;           // 方案/架构目录
        const std::filesystem::path& getEditorBigIconPath() const;      // 编辑器大图标路径
        const std::filesystem::path& getEditorSmallIconPath() const;    // 编辑器小图标路径
        const std::filesystem::path& getEditorFontPath() const;         // 编辑器字体路径

#ifdef ENABLE_PHYSICS_DEBUG_RENDERER
        const std::filesystem::path& getJoltPhysicsAssetFolder() const;
#endif

        const std::string& getDefaultWorldUrl() const;
        const std::string& getGlobalRenderingResUrl() const;

    private:
        std::filesystem::path m_root_folder;
        std::filesystem::path m_asset_folder;
        std::filesystem::path m_schema_folder;
        std::filesystem::path m_editor_big_icon_path;
        std::filesystem::path m_editor_small_icon_path;
        std::filesystem::path m_editor_font_path;

#ifdef ENABLE_PHYSICS_DEBUG_RENDERER
        std::filesystem::path m_jolt_physics_asset_folder;
#endif

        std::string m_default_world_url;        // 用于加载世界资源的 url ，牵扯到运算组件的 asset/world/hello.world.json
        std::string m_global_rendering_res_url;
    };
} // namespace Pilot
