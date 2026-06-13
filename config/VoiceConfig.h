#pragma once
#include <string>

struct VoiceConfig
{
    std::string tone;
    bool useEmoji;
    std::string length;
};

VoiceConfig getVoiceForPlatform(const std::string& platform);
