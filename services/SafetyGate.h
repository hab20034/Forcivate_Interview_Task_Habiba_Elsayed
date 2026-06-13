#pragma once

#include <string>

enum class RiskLevel
{
    SAFE,
    FLAGGED,
    HIGH_RISK
};

class SafetyGate
{
public:

    bool isSpam(const std::string& text);

    bool containsPromptInjection(const std::string& text);

    bool isHostile(const std::string& text);

    RiskLevel evaluate(const std::string& text);

    // Strips any injection directives from comment text before it reaches
    // the LLM. Called on ALL comments regardless of RiskLevel — even a
    // comment that scores SAFE may contain a partial injection phrase that
    // doesn't trip the keyword list but would still confuse the model.
    std::string sanitize(const std::string& text);

private:
    // Shared lowercase helper used by both detection and sanitization
    static std::string toLower(const std::string& text);
};