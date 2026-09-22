#include "MainIsLandScene.h"

#include "Engine/Module/GameObject/Transform/Transform.h"

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
        Coroutine::StartCoroutine(OnEnterAsync(BeginEnter()));
    }

    Coroutine::Task<void> MainIslandScene::OnEnterAsync(const int generation)
    {
        if (!co_await LoadMainSceneAsync(generation))
            co_return;

        // Context の FIELD は読み込んだシーン内を指すので、AddContent が済んだここで初めて触る
        Context()->Init();
        // メインシーンが居ない間に Instantiate が走らないよう、読み込みが済んでから積む
        SubScene().Push(Sub::SceneType::ChattingUI);
        
        auto loaded = Context()->PlayerAvatarFactory().LoadInitedPlayerAvatarWithAttachments(
            PlayerAvatar::LoadType(),
            Context()->PlayerSpawnPoint(),
            nullptr,
            true,
            std::make_shared<GameCore::PlayerAvatar::NullPlayerAvatarStatus>());
        playerAvatar_ = loaded.avatar;
        attachments_  = loaded.attachments;

        GamePlay::Sound::SoundPlayer::PlayBgm(Context()->BGM());
        CompleteEnter(generation);
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
        
    }

    void MainIslandScene::DoDispose()
    {
        // 読み込みの途中で抜けたときはアバターが居ない。そのときは進行も保存しない
        if (const auto avatar = playerAvatar_.lock())
        {
            PlayerAvatar::SaveType(*avatar);
            avatar->SaveStatus();
            SaveGameProgression(GameProgresion::GrassLandStage);
        }
        playerAvatar_.reset();
        attachments_ = {};
        
        GamePlay::Sound::SoundPlayer::StopBgm(Context()->BGM());
    }

    void MainIslandScene::OnDrawGui()
    {
        
    }
}
