#include "MainIsLandScene.h"

#include "../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"

#include "../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../../../Data/PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "../../../../PlayerAvatar/PlayerAvatar.h"
#include "../../../../PlayerAvatar/Status/NullPlayerAvatarStatus.h"
#include "../../../Sub/Group/Sub_IGameSceneGroup.h"
#include "../../../Sub/Type/SubSceneType.h"

namespace GameCore::Scene::Main
{
    MainIslandScene::MainIslandScene(
        const std::weak_ptr<MainIslandSceneContext>& context,
        GameSceneBaseContext baseContext)
        : GameMainSceneBase(context, baseContext)
    {
        
    }

    MainIslandScene::~MainIslandScene() = default;

    void MainIslandScene::Init()
    {
        SubScene().Push(Sub::SceneType::ChattingUI);
        
        scene_ = LoadMainScene();
        Context()->Init();
        
        auto loaded = Context()->PlayerAvatarFactory().LoadInitedPlayerAvatarWithAttachments(
            PlayerAvatar::LoadType(),
            Context()->PlayerSpawnPoint(),
            nullptr,
            true,
            std::make_shared<GameCore::PlayerAvatar::NullPlayerAvatarStatus>());
        playerAvatar_ = loaded.avatar;
        attachments_  = loaded.attachments;
    }

    void MainIslandScene::SwitchPlayerAvatar(const PlayerAvatar::PlayerAvatarType type)
    {
        const auto current = playerAvatar_.lock();
        if (!current || current->Type() == type)
            return;

        const auto position = current->PlayerTransform().GetWorldPos();
        current->SaveStatus();

        Context()->PlayerAvatarFactory().DestroyAttachments(attachments_);
        current->PlayerTransform().GetGameObject()->OnDestroy();
        playerAvatar_.reset();
        attachments_ = {};

        PlayerAvatar::SaveType(type);

        auto loaded = Context()->PlayerAvatarFactory().LoadInitedPlayerAvatarWithAttachments(
            type,
            position,
            nullptr,
            true,
            std::make_shared<GameCore::PlayerAvatar::NullPlayerAvatarStatus>());
        playerAvatar_ = loaded.avatar;
        attachments_  = loaded.attachments;
    }

    void MainIslandScene::Enter()
    {
        GamePlay::Sound::SoundPlayer::PlayBgm(Context()->BGM());
    }

    void MainIslandScene::DoDispose()
    {
        PlayerAvatar::SaveType(*playerAvatar_.lock());
        playerAvatar_.lock()->SaveStatus();
        SaveGameProgression(GameProgresion::GrassLandStage);
        
        GamePlay::Sound::SoundPlayer::StopBgm(Context()->BGM());
        Core::Application::ApplicationBase::GameWindow()->RemoveContent(scene_.lock());
    }

    void MainIslandScene::OnDrawGui()
    {
        
    }
}
