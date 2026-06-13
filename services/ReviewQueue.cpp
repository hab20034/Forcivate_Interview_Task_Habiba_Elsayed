#include "ReviewQueue.h"

#include <iostream>
#include <algorithm>    // std::tolower

std::string ReviewQueue::review(
    const std::string& comment,
    const std::string& draft)
{
    std::cout << "\nComment:\n";
    std::cout << comment << "\n";

    std::cout << "\nDraft:\n";
    std::cout << draft << "\n";

    // ── Action loop ────────────────────────────────────────────────────────
    // We loop until we get a valid choice so a stray Enter doesn't skip items.
    while (true)
    {
        std::cout << "\n(A)pprove  (E)dit  (R)eject : " << std::flush;

        std::string line;

        // KEY FIX: always use getline — never mix operator>> with getline.
        // operator>> leaves '\n' in the buffer; the next getline then reads
        // an empty string and appears to "skip" the prompt entirely.
        if (!std::getline(std::cin, line))
        {
            // EOF / stream error — treat as reject and stop processing.
            std::cout << "\n[Input stream closed. Rejecting remaining items.]\n";
            return "";
        }

        if (line.empty())
            continue;   // user just hit Enter — re-prompt instead of skipping

        char choice = static_cast<char>(
            std::tolower(static_cast<unsigned char>(line[0])));

        if (choice == 'a')
        {
            return draft;
        }

        if (choice == 'e')
        {
            std::cout << "Enter edited reply (press Enter when done):\n> " << std::flush;

            std::string edited;

            // No cin.ignore() needed — we're already using getline exclusively.
            if (!std::getline(std::cin, edited))
            {
                std::cout << "[Input stream closed. Keeping original draft.]\n";
                return draft;
            }

            // If the reviewer left it blank, keep the original draft.
            if (edited.empty())
            {
                std::cout << "[Empty input — keeping original draft.]\n";
                return draft;
            }

            return edited;
        }

        if (choice == 'r')
        {
            // FIX: 'R' previously fell through to return "" but left '\n'
            // in the buffer, which corrupted the very next cin >> call and
            // caused all subsequent comments to be silently rejected.
            // Now we just return "" cleanly after an explicit rejection.
            std::cout << "Rejected.\n";
            return "";
        }

        // Unrecognised input — tell the user and loop back.
        std::cout << "Please enter A, E, or R.\n";
    }
}