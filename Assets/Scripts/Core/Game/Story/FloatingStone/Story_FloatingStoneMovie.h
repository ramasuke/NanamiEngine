#pragma once
#include <functional>
#include <memory>

#include "Engine/Core/Coroutine/Task/Task.h"
#include "Libs/glm/vec3.hpp"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Asset
{
    class PrefabGameObjectFile;
}

namespace NanamiEngine::CineMachine
{
    class CineMachineVirtualCamera;
}

namespace GameCore::Story::FloatingStone
{
    /**
     * @brief 浮遊石の演出に使うシーンの物。どれかが無ければ演出を飛ばして結果だけにする
     * @note stone の子の ParticleSystem (オーラ) は石と一緒に隠す
     */
    struct StoneMovieCast
    {
        std::weak_ptr<IPlayerAvatar>                                  playerAvatar;
        std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> stone;
        /** LookAt で石を追うカメラ。シーンに置いた位置から動かない */
        std::shared_ptr<NanamiEngine::CineMachine::CineMachineVirtualCamera> camera;
        /** 飛んでいる石に重ねる光の尾 */
        std::shared_ptr<NanamiEngine::Module::Asset::PrefabGameObjectFile> flightParticle;
        /** 草原では抜け出す瞬間、拠点の島でははまる瞬間に出す */
        std::shared_ptr<NanamiEngine::Module::Asset::PrefabGameObjectFile> burstParticle;
    };

    /** @brief 石とその子のパーティクルを出す/隠す */
    void SetStoneVisible(NanamiEngine::Module::GameObject::IGameObject& stone, bool isVisible);

    /**
     * @brief 大顎を倒したあと、村の跡の石が震えて浮き上がり、空へ飛び去る。終わると石は隠れたまま
     * @param isCanceled シーンを抜けたら true にする。立っていれば何もせずに抜ける
     */
    Coroutine::Task<void> PlayDepartAsync(StoneMovieCast cast, std::shared_ptr<bool> isCanceled);

    /**
     * @brief 草原から戻った石が空から飛んできて、島の底にはまる。石のシーン上の位置がはまった位置
     * @param canStart 演出を始めてよいか。ロード画面が明けるまで false を返す (その裏で終わってしまうため)
     * @param onDocked はまった瞬間 (スキップしたときはその場) に一度だけ呼ぶ
     */
    Coroutine::Task<void> PlayReturnAsync(
        StoneMovieCast cast, std::shared_ptr<bool> isCanceled, std::function<bool()> canStart, std::function<void()> onDocked);

    /**
     * @brief 草原の後、緑の浮遊石の力で戻ってくる島とそこへ上る階段。どれかが無ければ演出を飛ばす
     * @note シーン上の位置が戻った位置。戻るまでは SinkIsland で雲の下へ退避させておく
     */
    struct IslandReturnCast
    {
        std::weak_ptr<IPlayerAvatar>                                  playerAvatar;
        std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> island;
        /** 子が1段ずつの足場。子の並び順に架かる */
        std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> stairs;
        /** LookAt で島を追うカメラ。シーンに置いた位置から動かない */
        std::shared_ptr<NanamiEngine::CineMachine::CineMachineVirtualCamera> camera;
        /** カメラが見る所。島の子(一緒に上がってくる物)にする */
        std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> focus;
    };

    /** @brief 戻る前の島と階段を隠し、コライダーごと雲の下へ退避させる。シーンに入ったときに一度だけ呼ぶ */
    void SinkIsland(const IslandReturnCast& cast);

    /** @brief 戻った島と階段を出す(シーンでは隠してある) */
    void ShowIsland(const IslandReturnCast& cast);

    /**
     * @brief SinkIsland で退避させた島が雲の下からせり上がり、階段が手前から1段ずつ架かる
     * @param canStart 演出を始めてよいか。ロード画面が明けるまで false を返す
     * @param onReturned 戻りきった瞬間 (スキップ・シーンを抜けたときはその場) に一度だけ呼ぶ
     */
    Coroutine::Task<void> PlayIslandReturnAsync(
        IslandReturnCast cast, std::shared_ptr<bool> isCanceled, std::function<bool()> canStart, std::function<void()> onReturned);

    /** @brief 序章で島の心臓(2つの浮遊石)が抜け出して散るときの尺と距離 */
    struct ScatterShot
    {
        float riseHeight   = 130.0f;
        float rise_secs    = 1.6f;
        float hover_secs   = 0.9f;
        float fly_secs     = 3.2f;
        float flyDistance  = 2600.0f; ///< 水平に飛ぶ距離
        float flyRise      = 700.0f;  ///< 飛ぶあいだに上がる高さ
    };

    /**
     * @brief stonesRoot の子(地面に埋めた浮遊石)をせり上がらせ、少し浮かせてから、根元から見た向きへそれぞれ飛ばす。
     *        石の子の ParticleSystem (光の尾。PlayMode は Manual にしておく) は飛び立つときに出す。飛び終えた石は隠す
     */
    Coroutine::Task<void> PlayScatterAsync(std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> stonesRoot, ScatterShot shot);
}
