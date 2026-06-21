#include "ApplicationConfiguration.h"
#include "../../../Module/ProjectConfig/Engine_Module_ProjectConfig.h"
#include "ImGuiHelper.h"

namespace NanamiEngine::Core::Application::Configuration
{
    constexpr auto DEFAULT_WINDOW_WIDTH_SIZE  = 1920;
    constexpr auto DEFAULT_WINDOW_HEIGHT_SIZE = 1080;
    constexpr auto DEFAULT_WINDOW_COLOR_SCALE = 16;
    constexpr auto DEFAULT_SHADOW_MAP_WIDTH   = 1024;
    constexpr auto DEFAULT_SHADOW_MAP_HEIGHT  = 1024;
    constexpr auto DEFAULT_LIGHT_DIR_X        = -0.5f;
    constexpr auto DEFAULT_LIGHT_DIR_Y        = -1.0f;
    constexpr auto DEFAULT_LIGHT_DIR_Z        = -0.5f;
    constexpr auto DEFAULT_LIGHT_DIF_R        = 1.0f;
    constexpr auto DEFAULT_LIGHT_DIF_G        = 1.0f;
    constexpr auto DEFAULT_LIGHT_DIF_B        = 1.0f;
    constexpr auto DEFAULT_PARTICLE_MAX             = 8000;
    constexpr auto DEFAULT_ASSETS_DIRECTORY_PATH   = "Assets";

    int   AppConfiguration::windowWidth_      = DEFAULT_WINDOW_WIDTH_SIZE;
    int   AppConfiguration::windowHeight_     = DEFAULT_WINDOW_HEIGHT_SIZE;
    int   AppConfiguration::windowColorScale_ = DEFAULT_WINDOW_COLOR_SCALE;
    int   AppConfiguration::shadowMapWidth_   = DEFAULT_SHADOW_MAP_WIDTH;
    int   AppConfiguration::shadowMapHeight_  = DEFAULT_SHADOW_MAP_HEIGHT;
    float AppConfiguration::lightDirX_        = DEFAULT_LIGHT_DIR_X;
    float AppConfiguration::lightDirY_        = DEFAULT_LIGHT_DIR_Y;
    float AppConfiguration::lightDirZ_        = DEFAULT_LIGHT_DIR_Z;
    float AppConfiguration::lightDifR_        = DEFAULT_LIGHT_DIF_R;
    float AppConfiguration::lightDifG_        = DEFAULT_LIGHT_DIF_G;
    float AppConfiguration::lightDifB_        = DEFAULT_LIGHT_DIF_B;
    int         AppConfiguration::particleMax_           = DEFAULT_PARTICLE_MAX;
    std::string AppConfiguration::assetsDirectoryPath_   = DEFAULT_ASSETS_DIRECTORY_PATH;

    constexpr auto APP_CONFIG_PATH            = "Application/";
    constexpr auto APP_CONFIG_WIDTH_KEY       = "WindowWidth";
    constexpr auto APP_CONFIG_HEIGHT_KEY      = "WindowHeight";
    constexpr auto APP_CONFIG_SCALE_KEY       = "WindowColorScale";
    constexpr auto APP_CONFIG_SHADOW_W_KEY    = "ShadowMapWidth";
    constexpr auto APP_CONFIG_SHADOW_H_KEY    = "ShadowMapHeight";
    constexpr auto APP_CONFIG_LIGHT_DX_KEY    = "LightDirX";
    constexpr auto APP_CONFIG_LIGHT_DY_KEY    = "LightDirY";
    constexpr auto APP_CONFIG_LIGHT_DZ_KEY    = "LightDirZ";
    constexpr auto APP_CONFIG_LIGHT_DR_KEY    = "LightDifR";
    constexpr auto APP_CONFIG_LIGHT_DG_KEY    = "LightDifG";
    constexpr auto APP_CONFIG_LIGHT_DB_KEY    = "LightDifB";
    constexpr auto APP_CONFIG_PARTICLE_MAX_KEY       = "ParticleMax";
    constexpr auto APP_CONFIG_ASSETS_DIR_PATH_KEY    = "AssetsDirectoryPath";

    void AppConfiguration::Load()
    {
        windowWidth_      = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (APP_CONFIG_PATH, APP_CONFIG_WIDTH_KEY,      DEFAULT_WINDOW_WIDTH_SIZE);
        windowHeight_     = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (APP_CONFIG_PATH, APP_CONFIG_HEIGHT_KEY,     DEFAULT_WINDOW_HEIGHT_SIZE);
        windowColorScale_ = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (APP_CONFIG_PATH, APP_CONFIG_SCALE_KEY,      DEFAULT_WINDOW_COLOR_SCALE);
        shadowMapWidth_   = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (APP_CONFIG_PATH, APP_CONFIG_SHADOW_W_KEY,   DEFAULT_SHADOW_MAP_WIDTH);
        shadowMapHeight_  = Module::ProjectConfig::LoadOrDefaultWithPath<int>  (APP_CONFIG_PATH, APP_CONFIG_SHADOW_H_KEY,   DEFAULT_SHADOW_MAP_HEIGHT);
        lightDirX_        = Module::ProjectConfig::LoadOrDefaultWithPath<float>(APP_CONFIG_PATH, APP_CONFIG_LIGHT_DX_KEY,   DEFAULT_LIGHT_DIR_X);
        lightDirY_        = Module::ProjectConfig::LoadOrDefaultWithPath<float>(APP_CONFIG_PATH, APP_CONFIG_LIGHT_DY_KEY,   DEFAULT_LIGHT_DIR_Y);
        lightDirZ_        = Module::ProjectConfig::LoadOrDefaultWithPath<float>(APP_CONFIG_PATH, APP_CONFIG_LIGHT_DZ_KEY,   DEFAULT_LIGHT_DIR_Z);
        lightDifR_        = Module::ProjectConfig::LoadOrDefaultWithPath<float>(APP_CONFIG_PATH, APP_CONFIG_LIGHT_DR_KEY,   DEFAULT_LIGHT_DIF_R);
        lightDifG_        = Module::ProjectConfig::LoadOrDefaultWithPath<float>(APP_CONFIG_PATH, APP_CONFIG_LIGHT_DG_KEY,   DEFAULT_LIGHT_DIF_G);
        lightDifB_        = Module::ProjectConfig::LoadOrDefaultWithPath<float>(APP_CONFIG_PATH, APP_CONFIG_LIGHT_DB_KEY,   DEFAULT_LIGHT_DIF_B);
        particleMax_         = Module::ProjectConfig::LoadOrDefaultWithPath<int>        (APP_CONFIG_PATH, APP_CONFIG_PARTICLE_MAX_KEY,    DEFAULT_PARTICLE_MAX);
        assetsDirectoryPath_ = Module::ProjectConfig::LoadOrDefaultWithPath<std::string>(APP_CONFIG_PATH, APP_CONFIG_ASSETS_DIR_PATH_KEY, std::string(DEFAULT_ASSETS_DIRECTORY_PATH));
    }

    void AppConfiguration::Save()
    {
        Module::ProjectConfig::SaveWithPath<int>  (APP_CONFIG_PATH, APP_CONFIG_WIDTH_KEY,      windowWidth_);
        Module::ProjectConfig::SaveWithPath<int>  (APP_CONFIG_PATH, APP_CONFIG_HEIGHT_KEY,     windowHeight_);
        Module::ProjectConfig::SaveWithPath<int>  (APP_CONFIG_PATH, APP_CONFIG_SCALE_KEY,      windowColorScale_);
        Module::ProjectConfig::SaveWithPath<int>  (APP_CONFIG_PATH, APP_CONFIG_SHADOW_W_KEY,   shadowMapWidth_);
        Module::ProjectConfig::SaveWithPath<int>  (APP_CONFIG_PATH, APP_CONFIG_SHADOW_H_KEY,   shadowMapHeight_);
        Module::ProjectConfig::SaveWithPath<float>(APP_CONFIG_PATH, APP_CONFIG_LIGHT_DX_KEY,   lightDirX_);
        Module::ProjectConfig::SaveWithPath<float>(APP_CONFIG_PATH, APP_CONFIG_LIGHT_DY_KEY,   lightDirY_);
        Module::ProjectConfig::SaveWithPath<float>(APP_CONFIG_PATH, APP_CONFIG_LIGHT_DZ_KEY,   lightDirZ_);
        Module::ProjectConfig::SaveWithPath<float>(APP_CONFIG_PATH, APP_CONFIG_LIGHT_DR_KEY,   lightDifR_);
        Module::ProjectConfig::SaveWithPath<float>(APP_CONFIG_PATH, APP_CONFIG_LIGHT_DG_KEY,   lightDifG_);
        Module::ProjectConfig::SaveWithPath<float>(APP_CONFIG_PATH, APP_CONFIG_LIGHT_DB_KEY,   lightDifB_);
        Module::ProjectConfig::SaveWithPath<int>        (APP_CONFIG_PATH, APP_CONFIG_PARTICLE_MAX_KEY,    particleMax_);
        Module::ProjectConfig::SaveWithPath<std::string>(APP_CONFIG_PATH, APP_CONFIG_ASSETS_DIR_PATH_KEY, assetsDirectoryPath_);
    }

    int   AppConfiguration::GetWindowWidth()        { return windowWidth_; }
    int   AppConfiguration::GetWindowHeight()       { return windowHeight_; }
    int   AppConfiguration::GetWindowColorScale()   { return windowColorScale_; }
    void  AppConfiguration::SetWindowWidth(int w)   { windowWidth_      = w; }
    void  AppConfiguration::SetWindowHeight(int h)  { windowHeight_     = h; }
    void  AppConfiguration::SetWindowColorScale(int s) { windowColorScale_ = s; }

    int   AppConfiguration::GetShadowMapWidth()         { return shadowMapWidth_; }
    int   AppConfiguration::GetShadowMapHeight()        { return shadowMapHeight_; }
    void  AppConfiguration::SetShadowMapWidth(int w)    { shadowMapWidth_   = w; }
    void  AppConfiguration::SetShadowMapHeight(int h)   { shadowMapHeight_  = h; }

    float AppConfiguration::GetLightDirX()              { return lightDirX_; }
    float AppConfiguration::GetLightDirY()              { return lightDirY_; }
    float AppConfiguration::GetLightDirZ()              { return lightDirZ_; }
    void  AppConfiguration::SetLightDirX(float x)       { lightDirX_ = x; }
    void  AppConfiguration::SetLightDirY(float y)       { lightDirY_ = y; }
    void  AppConfiguration::SetLightDirZ(float z)       { lightDirZ_ = z; }

    float AppConfiguration::GetLightDifR()              { return lightDifR_; }
    float AppConfiguration::GetLightDifG()              { return lightDifG_; }
    float AppConfiguration::GetLightDifB()              { return lightDifB_; }
    void  AppConfiguration::SetLightDifR(float r)       { lightDifR_ = r; }
    void  AppConfiguration::SetLightDifG(float g)       { lightDifG_ = g; }
    void  AppConfiguration::SetLightDifB(float b)       { lightDifB_ = b; }

    int   AppConfiguration::GetParticleMax()        { return particleMax_; }
    void  AppConfiguration::SetParticleMax(int max) { particleMax_ = max; }

    const std::string& AppConfiguration::GetAssetsDirectoryPath()                  { return assetsDirectoryPath_; }
    void               AppConfiguration::SetAssetsDirectoryPath(const std::string& path) { assetsDirectoryPath_ = path; }

    void AppConfiguration::DrawConfigGUI()
    {
        ImGui::Text("Application");
        ImGui::Separator();

        int w = GetWindowWidth();
        int h = GetWindowHeight();
        int s = GetWindowColorScale();

        ImGui::SetNextItemWidth(100);
        const bool wChanged = ImGui::InputInt("Window Width",  &w);
        ImGui::SetNextItemWidth(100);
        const bool hChanged = ImGui::InputInt("Window Height", &h);
        ImGui::SetNextItemWidth(100);
        const bool sChanged = ImGui::InputInt("Color Scale",   &s);

        if (wChanged || hChanged || sChanged)
        {
            SetWindowWidth(w);
            SetWindowHeight(h);
            SetWindowColorScale(s);
            Save();
        }
        ImGui::TextDisabled("* Restart required to apply");

        ImGui::Spacing();
        ImGui::Text("Shadow Map");
        ImGui::Separator();

        int sw = GetShadowMapWidth();
        int sh = GetShadowMapHeight();

        ImGui::SetNextItemWidth(100);
        const bool swChanged = ImGui::InputInt("Shadow Map Width",  &sw);
        ImGui::SetNextItemWidth(100);
        const bool shChanged = ImGui::InputInt("Shadow Map Height", &sh);

        if (swChanged || shChanged)
        {
            SetShadowMapWidth(sw);
            SetShadowMapHeight(sh);
            Save();
        }
        ImGui::TextDisabled("* Restart required to apply");

        ImGui::Spacing();
        ImGui::Text("Light");
        ImGui::Separator();

        float dir[3] = { GetLightDirX(), GetLightDirY(), GetLightDirZ() };
        float dif[3] = { GetLightDifR(), GetLightDifG(), GetLightDifB() };

        ImGui::SetNextItemWidth(200);
        const bool dirChanged = ImGui::InputFloat3("Light Direction", dir);
        ImGui::SetNextItemWidth(200);
        const bool difChanged = ImGui::ColorEdit3 ("Light Diffuse",   dif);

        if (dirChanged)
        {
            SetLightDirX(dir[0]);
            SetLightDirY(dir[1]);
            SetLightDirZ(dir[2]);
            Save();
        }
        if (difChanged)
        {
            SetLightDifR(dif[0]);
            SetLightDifG(dif[1]);
            SetLightDifB(dif[2]);
            Save();
        }
        ImGui::TextDisabled("* Restart required to apply");

        ImGui::Spacing();
        ImGui::Text("Effekseer");
        ImGui::Separator();

        int particleMax = GetParticleMax();
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputInt("Particle Max", &particleMax))
        {
            if (particleMax < 1) particleMax = 1;
            SetParticleMax(particleMax);
            Save();
        }
        ImGui::TextDisabled("* Restart required to apply");

        ImGui::Spacing();
        ImGui::Text("Assets");
        ImGui::Separator();

        char assetsPathBuf[256] = {};
        snprintf(assetsPathBuf, sizeof(assetsPathBuf), "%s", assetsDirectoryPath_.c_str());
        ImGui::SetNextItemWidth(200);
        if (ImGui::InputText("Assets Directory", assetsPathBuf, sizeof(assetsPathBuf)))
        {
            assetsDirectoryPath_ = assetsPathBuf;
            Save();
        }
        ImGui::TextDisabled("* Use 'Reload Assets' to apply");

        ImGui::Spacing();
    }
}
