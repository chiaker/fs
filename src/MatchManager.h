#pragma once
#include "User.h"
#include "ProfileManager.h"
#include <string>
#include <mutex>

class MatchManager {
public:
    MatchManager(ProfileManager &pm);
    bool like(int userId, int targetId, std::string &outMsg);
    int totalMatches() const;

private:
    ProfileManager &profiles;
    int matchCount;
    mutable std::mutex mtx;
};
