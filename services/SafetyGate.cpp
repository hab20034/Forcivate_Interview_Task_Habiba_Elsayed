#include "SafetyGate.h"

#include <algorithm>
#include <regex>

// ── Private helper ─────────────────────────────────────────────────────────
std::string SafetyGate::toLower(const std::string& text)
{
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// ── Detection methods ──────────────────────────────────────────────────────

bool SafetyGate::isSpam(const std::string& text)
{
    std::string lower = toLower(text);
    return lower.find("$5000") != std::string::npos
        || lower.find("check my profile") != std::string::npos
        || lower.find("no risk") != std::string::npos
        || lower.find("make money") != std::string::npos;
}

bool SafetyGate::containsPromptInjection(const std::string& text)
{
    // FIX: was case-sensitive, so "ignore all previous instructions" (lowercase)
    // slipped straight through. All checks are now done on a lowercased copy.
    std::string lower = toLower(text);

    return lower.find("ignore all previous instructions") != std::string::npos
        || lower.find("ignore previous instructions")     != std::string::npos
        || lower.find("disregard all previous")           != std::string::npos
        || lower.find("output exactly")                   != std::string::npos
        || lower.find("output only")                      != std::string::npos
        || lower.find("respond only with")                != std::string::npos
        || lower.find("reply with exactly")               != std::string::npos
        || lower.find("do not generate")                  != std::string::npos
        || lower.find("only output that phrase")          != std::string::npos
        || lower.find("forget your instructions")         != std::string::npos
        || lower.find("new instructions:")                != std::string::npos
        || lower.find("system prompt:")                   != std::string::npos;
}

bool SafetyGate::isHostile(const std::string& text)
{
    std::string lower = toLower(text);
    return lower.find("garbage")    != std::string::npos
        || lower.find("ashamed")    != std::string::npos
        || lower.find("incompetent")!= std::string::npos
        || lower.find("pathetic")   != std::string::npos
        || lower.find("useless")    != std::string::npos
        || lower.find("idiots")     != std::string::npos;
}

RiskLevel SafetyGate::evaluate(const std::string& text)
{
    if (isSpam(text))
        return RiskLevel::FLAGGED;

    if (containsPromptInjection(text))
        return RiskLevel::FLAGGED;

    if (isHostile(text))
        return RiskLevel::HIGH_RISK;

    return RiskLevel::SAFE;
}

// ── Sanitize ───────────────────────────────────────────────────────────────
// Removes prompt-structure keywords that could cause the LLM to treat
// comment text as instructions rather than data to respond to.
// This runs on ALL comments — detection (evaluate) decides whether to skip
// drafting entirely, but sanitize() is an extra hardening layer that strips
// any residual directive language before the text is forwarded to the LLM.
std::string SafetyGate::sanitize(const std::string& text)
{
    // List of patterns that directly map to LLM prompt-control language.
    // Using regex with case-insensitive flag so we catch all capitalizations.
    static const std::vector<std::regex> injectionPatterns = {
        std::regex(R"(ignore\s+all\s+previous\s+instructions?)",  std::regex::icase),
        std::regex(R"(ignore\s+previous\s+instructions?)",        std::regex::icase),
        std::regex(R"(disregard\s+all\s+previous)",               std::regex::icase),
        std::regex(R"(output\s+exactly)",                         std::regex::icase),
        std::regex(R"(output\s+only)",                            std::regex::icase),
        std::regex(R"(respond\s+only\s+with)",                    std::regex::icase),
        std::regex(R"(reply\s+with\s+exactly)",                   std::regex::icase),
        std::regex(R"(do\s+not\s+generate\s+a\s+helpful)",        std::regex::icase),
        std::regex(R"(only\s+output\s+that\s+phrase)",            std::regex::icase),
        std::regex(R"(forget\s+your\s+instructions)",             std::regex::icase),
        std::regex(R"(new\s+instructions\s*:)",                   std::regex::icase),
        std::regex(R"(system\s+prompt\s*:)",                      std::regex::icase),
        // Also strip bare prompt-structure labels so the model can't be
        // tricked by a comment that embeds "Post:" or "Comment:" to hijack
        // the few-shot template format in llm_adapter.py
        std::regex(R"(^\s*Post\s*:)",                             std::regex::icase),
        std::regex(R"(^\s*Reply\s*:)",                            std::regex::icase),
        std::regex(R"(^\s*Comment\s*:)",                          std::regex::icase),
    };

    std::string sanitized = text;
    for (const auto& pattern : injectionPatterns)
    {
        sanitized = std::regex_replace(sanitized, pattern, "[removed]");
    }

    return sanitized;
}