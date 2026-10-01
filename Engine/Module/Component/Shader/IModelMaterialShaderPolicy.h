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

    // 同じ GameObject の兄弟コンポーネントが実装し、材質名ごとの描画パスを ModelRenderer に渡す
    class NANAMI_API IModelMaterialShaderPolicy
    {
    public:
        virtual ~IModelMaterialShaderPolicy() = default;

        // 引き受けるなら outPass を埋めて true を返す。定数バッファもここで更新する (通常パス専用)
        [[nodiscard]] virtual bool TryGetMaterialShaderPass(const std::string& materialName, MaterialShaderPass& outPass) = 0;

        // 影パス専用。定数バッファの更新が二重に走らないよう、副作用を持たせないこと。
        [[nodiscard]] virtual bool ShouldDrawShadow(const std::string& materialName) = 0;
    };
}
