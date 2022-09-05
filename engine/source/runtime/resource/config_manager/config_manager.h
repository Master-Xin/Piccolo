#pragma once

#include <filesystem>

namespace Pilot
{
    struct EngineInitParams;

    class ConfigManager
    {
    public:
        void initialize(const EngineInitParams& init_param);

        const std::filesystem::path& getRootFolder() const;             // ��Ŀ¼
        const std::filesystem::path& getAssetFolder() const;            // �ʲ�Ŀ¼
        const std::filesystem::path& getSchemaFolder() const;           // ����/�ܹ�Ŀ¼
        const std::filesystem::path& getEditorBigIconPath() const;      // �༭����ͼ��·��
        const std::filesystem::path& getEditorSmallIconPath() const;    // �༭��Сͼ��·��
        const std::filesystem::path& getEditorFontPath() const;         // �༭������·��

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

        std::string m_default_world_url;        // ���ڼ���������Դ�� url ��ǣ������������� asset/world/hello.world.json
        std::string m_global_rendering_res_url;
    };
} // namespace Pilot
