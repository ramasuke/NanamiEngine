#pragma once

namespace GamePlay::Prop::StoryMovie
{
    // NOTE: 押しっぱなしで入ってきても即スキップにならないよう、一度離すまで待つ
    class SkipInput final
    {
    public:
        bool IsSkipped();

    private:
        bool isArmed_ = false;
    };
}
