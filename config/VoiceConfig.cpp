#include "VoiceConfig.h"

VoiceConfig getVoiceForPlatform(const std::string& platform)
{
    VoiceConfig voice;

    if (platform == "shortform")
    {
        voice.tone = "casual";
        voice.useEmoji = true;
        voice.length = "short";
    }
    else if (platform == "professional")
    {
        voice.tone = "professional";
        voice.useEmoji = false;
        voice.length = "medium";
    }
    else
    {
        voice.tone = "neutral";
        voice.useEmoji = false;
        voice.length = "medium";
    }

    return voice;
}