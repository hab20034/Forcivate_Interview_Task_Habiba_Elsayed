#pragma once

#include <string>
#include <vector>
#include "Comment.h"

struct Post
{
    std::string postId;
    std::string platform;
    std::string postText;

    std::vector<Comment> comments;
};