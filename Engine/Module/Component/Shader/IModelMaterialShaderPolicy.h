#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>

#include "../../../../Libs/LibCore/DxLib/BlendMode.h"

namespace NanamiEngine::Module::Component
{
    // ModelRendererが材質ごとの描画切り替えに使う受け渡し用データ。
    struct NANAMI_API MaterialShaderPass
    {
        int  vsHandle       = -1;
        int  psHandle       = -1;
        int  cbHandle       = -1;    // b4に積む定数バッファ。ポリシー側が生成・更新済みであること
        LibCore::Dxlib::BlendMode blendMode = LibCore::Dxlib::BlendMode::NoBlend;
        int  blendParam     = 0;
        bool disableZWrite  = false; // DxLibのZ書き込み制御はモデル単位しか無いため、1つでもtrueならモデル全体がOFFになる
        bool disableCulling = false; // この材質を使うメッシュを両面描画にする
    };

    // ModelRendererは「材質」は知るが「用途」は知らない。同じGameObject上の兄弟コンポーネントがこれを実装し、
    // 材質名ごとの描画パスを供給する(ComponentGroup::Catches<IModelMaterialShaderPolicy>()で発見される)。
    class NANAMI_API IModelMaterialShaderPolicy
    {
    public:
        virtual ~IModelMaterialShaderPolicy() = default;

        // materialNameの描画を引き受けるならtrueを返しoutPassを埋める。定数バッファの更新もここで行う。
        // 通常描画パスからのみ呼ばれる。
        [[nodiscard]] virtual bool TryGetMaterialShaderPass(const std::string& materialName, MaterialShaderPass& outPass) = 0;

        // 影パス専用。定数バッファの更新が二重に走らないよう、副作用を持たせないこと。
        [[nodiscard]] virtual bool ShouldDrawShadow(const std::string& materialName) = 0;
    };
}
