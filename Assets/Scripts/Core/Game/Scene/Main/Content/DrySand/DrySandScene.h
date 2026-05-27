// #pragma once
//
// namespace GameCore::Scene::Main
// {
//     class DrySandScene final : public GameMainSceneBase<GrassLandSceneContext>
//     {
//     public:
//         explicit GrassLandScene(
//             const std::weak_ptr<GrassLandSceneContext>& context,
//             GameSceneBaseContext baseContext);
//         ~GrassLandScene() override;
//         
//     private:
//         void Init     () override;
//         void Enter    () override;
//         void Dispose  () override;
//         void OnDrawGui() override;
//         
//         std::weak_ptr<NanamiEngine::Scene::Scene> scene_;
//         std::weak_ptr<IPlayerAvatar> playerAvatar_;
//     };
// }
