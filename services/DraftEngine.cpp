#include "DraftEngine.h"

#include <pybind11/embed.h>
#include <filesystem>
#include <iostream>

namespace py = pybind11;

static std::string getPythonModulePath()
{
    std::filesystem::path sourceFile = std::filesystem::absolute(__FILE__);
    auto modulePath = sourceFile.parent_path().parent_path() / "python";
    return modulePath.lexically_normal().string();
}

DraftEngine::DraftEngine()
{
    py::initialize_interpreter();

    try
    {
        py::module sys = py::module::import("sys");
        sys.attr("path").attr("insert")(0, getPythonModulePath());
    }
    catch (const py::error_already_set& e)
    {
        std::cerr << "Failed to initialize Python sys.path: " << e.what() << std::endl;
    }
}

std::string DraftEngine::generateDraft(
    const std::string& post,
    const std::string& comment,
    const VoiceConfig& voice)
{
    try
    {
        py::module llm =
            py::module::import("llm_adapter");

        auto fn = llm.attr("generate_reply");

        return fn(
            post,
            comment,
            voice.tone,
            voice.useEmoji,
            voice.length
        ).cast<std::string>();
    }
    catch (const py::error_already_set& e)
    {
        std::cerr << "Python error in generateDraft: " << e.what() << std::endl;
        return "";
    }
}