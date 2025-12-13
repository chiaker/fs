#include "MatchManager.h"
#include <algorithm>

MatchManager::MatchManager(ProfileManager &pm) : profiles(pm), matchCount(0) {
    // Count existing matches on load
    for (const auto &u : profiles.listUsers()) matchCount += (int)u.matches.size();
}

bool MatchManager::like(int userId, int targetId, std::string &outMsg, bool isSuper) {
    if (userId == targetId) { outMsg = "cannot like self"; return false; }
    // lock profiles while accessing users
    auto lock = profiles.getLock();
    User* u = profiles.getUserPtrLocked(userId);
    User* t = profiles.getUserPtrLocked(targetId);
    if (!u || !t) { outMsg = "user or target not found"; return false; }

    // Prevent duplicate likes; also record superlike if requested
    if (isSuper) {
        if (std::find(u->superlikes.begin(), u->superlikes.end(), targetId) == u->superlikes.end()) {
            u->superlikes.push_back(targetId);
        }
        // also treat superlike as a regular like for matching logic and future checks
        if (std::find(u->likes.begin(), u->likes.end(), targetId) == u->likes.end()) {
            u->likes.push_back(targetId);
        }
    } else {
        if (std::find(u->likes.begin(), u->likes.end(), targetId) == u->likes.end()) {
            u->likes.push_back(targetId);
        }
    }

    // Check if reciprocal like or superlike
    bool targetLikedBack = (std::find(t->likes.begin(), t->likes.end(), userId) != t->likes.end());
    bool targetSuperlikedBack = (std::find(t->superlikes.begin(), t->superlikes.end(), userId) != t->superlikes.end());
    if (targetLikedBack || targetSuperlikedBack) {
        // It's a match if not already recorded
        if (std::find(u->matches.begin(), u->matches.end(), targetId) == u->matches.end()) {
            u->matches.push_back(targetId);
            t->matches.push_back(userId);
            {
                std::lock_guard<std::mutex> lk(mtx);
                ++matchCount;
            }
            profiles.save();
            if (isSuper || targetSuperlikedBack) outMsg = "super match! contacts revealed: " + t->contact + " <-> " + u->contact;
            else outMsg = "match! contacts revealed: " + t->contact + " <-> " + u->contact;
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
