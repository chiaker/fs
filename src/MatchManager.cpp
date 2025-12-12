#include "MatchManager.h"
#include <algorithm>

MatchManager::MatchManager(ProfileManager &pm) : profiles(pm), matchCount(0) {
    // Count existing matches on load
    for (const auto &u : profiles.listUsers()) matchCount += (int)u.matches.size();
}

bool MatchManager::like(int userId, int targetId, std::string &outMsg) {
    if (userId == targetId) { outMsg = "cannot like self"; return false; }
    // lock profiles while accessing users
    auto lock = profiles.getLock();
    User* u = profiles.getUserPtrLocked(userId);
    User* t = profiles.getUserPtrLocked(targetId);
    if (!u || !t) { outMsg = "user or target not found"; return false; }

    // Prevent duplicate likes
    if (std::find(u->likes.begin(), u->likes.end(), targetId) == u->likes.end()) {
        u->likes.push_back(targetId);
    }

    // Check if reciprocal
    if (std::find(t->likes.begin(), t->likes.end(), userId) != t->likes.end()) {
        // It's a match if not already recorded
        if (std::find(u->matches.begin(), u->matches.end(), targetId) == u->matches.end()) {
            u->matches.push_back(targetId);
            t->matches.push_back(userId);
            {
                std::lock_guard<std::mutex> lk(mtx);
                ++matchCount;
            }
            profiles.save();
            outMsg = "match! contacts revealed: " + t->contact + " <-> " + u->contact;
            return true;
        } else {
            outMsg = "already matched";
            return false;
        }
    }
    profiles.save();
    outMsg = "like recorded";
    return false;
}

int MatchManager::totalMatches() const { return matchCount; }
