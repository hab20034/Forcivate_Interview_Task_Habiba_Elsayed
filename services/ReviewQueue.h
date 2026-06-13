#pragma once

#include <string>

class ReviewQueue
{
public:

    std::string review(
        const std::string& comment,
        const std::string& draft);
};