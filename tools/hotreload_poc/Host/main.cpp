// HotReload PoC の Host (docs/HotReload.md §10 PoC)。
// Game.dll を一意名にコピーしてロード → エンジン側でゲーム型を保存・復元 → ゲーム側でエンジン型を保存・復元
// → 登録記録から cereal の表を掃除 → アンロード、を CYCLES 回繰り返し、表の大きさが毎回同じに戻ることを確かめる。
// 終了コード 0 = PASS。
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#if defined(_WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#include "PocPlatform.h"
#include "../Engine/PocEngine.h"
#include "../Game/PocGameApi.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationModuleUnloader.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationTypeRegistry.h"
#include "Engine/Module/Serialization/Engine_Module_SharedStaticObject.h"

namespace
{
    int failures = 0;

    void Check(const bool condition, const char* what)
    {
        std::printf("  [%s] %s\n", condition ? " ok " : "FAIL", what);
        if (!condition)
            ++failures;
    }

    bool Contains(const std::string& text, const char* needle) { return text.find(needle) != std::string::npos; }

    void PrintStats(const char* label, const Poc::Stats& s)
    {
        std::printf("  %-22s in(json %zu, bin %zu) out(json %zu, bin %zu) casters(bases %zu, entries %zu, reverse %zu) versions %zu shared %zu records %zu\n",
            label, s.inputJson, s.inputBinary, s.outputJson, s.outputBinary, s.casterBases, s.casterEntries, s.reverseEntries, s.versions, s.sharedStatics, s.records);
    }
}

int main(int argc, char** argv)
{
    using namespace NanamiEngine::Module::Serialization;
    std::setvbuf(stdout, nullptr, _IONBF, 0); // 途中で落ちても出力が残るように
#if defined(_WIN32) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    const std::string gameLibrary = argc > 1 ? argv[1] : std::string("HotReloadPocGame") + Poc::Platform::LibraryExtension();
    const int cycles = argc > 2 ? std::atoi(argv[2]) : 10;

    // エンジンだけが登録されている状態 (ゲーム型のアンロード後に戻るべき形)
    const Poc::Stats baseline = Poc::Engine::GetStats();
    PrintStats("baseline", baseline);
    Poc::Stats afterUnload = baseline;

    for (int cycle = 1; cycle <= cycles; ++cycle)
    {
        std::printf("--- cycle %d ---\n", cycle);
        // 8. リンカが元ファイルを上書きできるように一意名で読む
        const std::string copy = "HotReloadPocGame_" + std::to_string(cycle) + Poc::Platform::LibraryExtension();
        Check(Poc::Platform::CopyFileTo(gameLibrary, copy), "copy Game library to a unique name");
        const auto handle = Poc::Platform::Load(copy);
        if (!handle)
        {
            std::printf("  load failed: %s\n", Poc::Platform::LastError().c_str());
            return 2;
        }
        auto* getApi = reinterpret_cast<const Poc::GameApi* (*)()>(Poc::Platform::Symbol(handle, "PocGetGameApi"));
        Check(getApi != nullptr, "PocGetGameApi exported");
        if (!getApi)
            return 2;
        const Poc::GameApi* api = getApi();
        // 登録記録の module は「登録子のアドレスが属するモジュール」。ロードした DLL と同じでなければならない
        const void* module = SerializationTypeRegistry::ModuleOf(reinterpret_cast<const void*>(getApi));
#if defined(_WIN32)
        Check(module == handle, "ModuleOf(exported function) == LoadLibrary handle");
#endif
        const auto records = SerializationTypeRegistry::Instance().RecordsOfModule(module);
        Check(records.size() == 3, "Game registered 3 records (2 types + 1 extra relation)");
        Check(records.size() >= 1 && records[0].name == "Poc::Game::GameComponent", "polymorphic_name is the macro token text");
        PrintStats("after load", Poc::Engine::GetStats());

        {
            // 1. エンジン側のコードでゲーム型を保存・復元 (= Scene ロード)。JSON と PortableBinary
            auto gameComponent = api->CreateGameComponent();
            const std::string json = Poc::Engine::SaveJson(gameComponent);
            Check(Contains(json, "\"polymorphic_name\": \"Poc::Game::GameComponent\""), "engine SaveJson writes the game type's name");
            auto loaded = Poc::Engine::LoadJson(json);
            Check(loaded && Contains(Poc::Engine::TypeName(*loaded), "GameComponent"), "engine LoadJson restores the game type");
            Check(loaded && api->Tick(*loaded) == 11, "dynamic_cast to the 2nd base (IUpdatable) works across the boundary");
            Check(loaded && dynamic_cast<Poc::EngineComponent*>(loaded.get()) != nullptr && dynamic_cast<Poc::EngineComponent*>(loaded.get())->speed == 3.0f,
                  "host dynamic_cast to the engine base of a game type");

            auto gameOnly = api->CreateGameOnly();
            const std::string bytes = Poc::Engine::SaveBinary(gameOnly);
            auto loadedOnly = Poc::Engine::LoadBinary(bytes);
            Check(loadedOnly && Contains(Poc::Engine::TypeName(*loadedOnly), "GameOnly") && loadedOnly->id == 2, "engine PortableBinary round-trips a game type");

            // 2. ゲーム側のコードでエンジン型を保存・復元 (ゲームから見てもエンジンの登録が見える)
            auto engineComponent = Poc::Engine::CreateEngineComponent();
            const std::string fromGame = api->SaveFromGame(engineComponent);
            Check(Contains(fromGame, "\"polymorphic_name\": \"Poc::EngineComponent\""), "game SaveFromGame writes the engine type's name");
            auto loadedInGame = api->LoadFromGame(fromGame);
            Check(loadedInGame && Contains(Poc::Engine::TypeName(*loadedInGame), "EngineComponent") && loadedInGame->id == 7, "game LoadFromGame restores the engine type");

            // 3. ゲーム型のインスタンスはこのスコープを抜けて全部消える (実機ではシーン破棄に相当)
        }

        // 4. 記録から cereal の表を掃除 (FreeLibrary の前)
        const ModuleUnloadReport report = SerializationModuleUnloader::Unregister(module);
        std::printf("  unregister: in %zu out %zu casters %zu swept %zu records %zu shared %zu\n",
            report.inputBindings, report.outputBindings, report.casters, report.sweptCasters, report.records, report.sharedStatics);
        Check(report.inputBindings == 4 && report.outputBindings == 4, "2 types x 2 archives removed from input and output maps");
        Check(report.records == 3, "3 records removed");
        Check(SerializationTypeRegistry::Instance().RecordsOfModule(module).empty(), "no records left for the module");
        Check(SerializationModuleUnloader::CountLeftoverCasters(module) == 0, "no caster with a vtable in the module left");
        Check(SharedStaticObjects::CountOwnedBy(module) == 0, "no shared StaticObject owned by the module left");

        // 5. アンロード、Versions を消す
        Check(Poc::Platform::Unload(handle), "unload");
        SerializationModuleUnloader::ClearClassVersions();
        Poc::Platform::DeleteFileAt(copy);

        const Poc::Stats now = Poc::Engine::GetStats();
        PrintStats("after unload", now);
        Check(now.inputJson == baseline.inputJson && now.inputBinary == baseline.inputBinary
              && now.outputJson == baseline.outputJson && now.outputBinary == baseline.outputBinary, "binding maps back to baseline");
        Check(now.casterBases == baseline.casterBases && now.casterEntries == baseline.casterEntries && now.reverseEntries == baseline.reverseEntries, "casters back to baseline");
        Check(now.records == baseline.records && now.sharedStatics == baseline.sharedStatics, "records and shared statics back to baseline");
        if (cycle > 1)
            Check(now == afterUnload, "identical to the previous cycle's after-unload state");
        afterUnload = now;

        // 6. アンロード後もエンジン型の保存・復元は壊れていない
        auto engineComponent = Poc::Engine::CreateEngineComponent();
        auto reloaded = Poc::Engine::LoadJson(Poc::Engine::SaveJson(engineComponent));
        Check(reloaded && reloaded->id == 7, "engine type still round-trips after unload");
    }

    std::printf("%s (%d cycles, %d failures)\n", failures == 0 ? "PASS" : "FAIL", cycles, failures);
    return failures == 0 ? 0 : 1;
}
