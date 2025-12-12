#pragma once
#include "User.h"
#include <vector>
#include <map>
#include <string>
#include <mutex>

class ProfileManager {
public:
    ProfileManager(const std::string &storagePath);
    ~ProfileManager();

    User createUser(const std::string &name, int age, const std::string &bio, const std::string &contact, const std::string &photo = "");
    std::vector<User> listUsers() const;
    bool save();
    bool load();
    // Returns a pointer to internal User while the returned lock is held.
    std::unique_lock<std::mutex> getLock();
    User* getUserPtrLocked(int id);
    int totalUsers() const;

private:
    std::vector<User> users;
    std::map<int, size_t> idToIndex;
    std::string path;
    int nextId;
    void rebuildIndex();
    mutable std::mutex mtx;
};
