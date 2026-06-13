#pragma once

#include "../config/VoiceConfig.h"
#include <string>

class DraftEngine
{
public:

    DraftEngine();

    std::string generateDraft(
        const std::string& post,
        const std::string& comment,
        const VoiceConfig& voice
    );
};