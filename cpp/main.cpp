#include <iostream>
#include <fstream>
#include <filesystem>

#include "../json.hpp"

#include "services/DraftEngine.h"
#include "services/SafetyGate.h"
#include "services/ReviewQueue.h"
#include "config/VoiceConfig.h"

using json = nlohmann::json;

int main()
{
    // Load comments dataset
    std::filesystem::path dataFile =
        std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "comments.json";

    std::ifstream file(dataFile);

    if (!file.is_open())
    {
        std::cerr << "Failed to open comments.json at " << dataFile << "\n";
        return 1;
    }

    json data;
    file >> data;

    // Create services
    DraftEngine engine;
    SafetyGate gate;
    ReviewQueue queue;

    json draftsOutput = json::array();
    json approvalsOutput = json::array();
    json publishedOutput = json::array();

    // Process posts
    for (const auto& post : data)
    {
        std::string postId = post["post_id"];
        std::string platform = post["platform"];
        std::string postText = post["post_text"];

        VoiceConfig voice =
            getVoiceForPlatform(platform);

        std::cout << "\n=====================================\n";
        std::cout << "Processing Post: "
                  << postId
                  << "\n";
        std::cout << "=====================================\n";

        for (const auto& comment : post["comments"])
        {
            std::string commentId =
                comment["comment_id"];

            std::string author =
                comment["author"];

            std::string commentText =
                comment["text"];

            std::cout << "\n-------------------------------------\n";
            std::cout << "Comment ID: "
                      << commentId
                      << "\n";

            RiskLevel risk =
                gate.evaluate(commentText);

            // Skip dangerous comments
            if (risk == RiskLevel::FLAGGED)
            {
                std::cout
                    << "Flagged by safety gate. Skipping.\n";

                approvalsOutput.push_back(
                {
                    {"comment_id", commentId},
                    {"decision", "flagged"},
                    {"reason", "Safety Gate"}
                });

                continue;
            }

            std::string draft;

            std::string safeCommentText = gate.sanitize(commentText);

            // Hostile comments get a safe canned response
            if (risk == RiskLevel::HIGH_RISK)
            {
                draft =
                    "We're sorry to hear about your experience. "
                    "Please contact our support team so we can investigate further.";
            }
            else
            {
                draft =
                    engine.generateDraft(
                        postText,
                        safeCommentText,
                        voice
                    );
            }

            draftsOutput.push_back(
            {
                {"comment_id", commentId},
                {"draft", draft}
            });

            std::string reviewedReply =
                queue.review(
                    commentText,
                    draft
                );

            if (reviewedReply.empty())
            {
                approvalsOutput.push_back(
                {
                    {"comment_id", commentId},
                    {"decision", "rejected"}
                });

                continue;
            }

            approvalsOutput.push_back(
            {
                {"comment_id", commentId},
                {"decision", "approved"},
                {"reply", reviewedReply}
            });

            // Mock publish step
            publishedOutput.push_back(
            {
                {"comment_id", commentId},
                {"published_reply", reviewedReply}
            });

            std::cout
                << "Published successfully.\n";
        }
    }

    // Save drafts
    {
        std::ofstream out("output/drafts.json");
        out << draftsOutput.dump(4);
    }

    // Save approvals
    {
        std::ofstream out("output/approvals.json");
        out << approvalsOutput.dump(4);
    }

    // Save published replies
    {
        std::ofstream out("output/published.json");
        out << publishedOutput.dump(4);
    }

    std::cout
        << "\nProcessing complete.\n";

    return 0;
}