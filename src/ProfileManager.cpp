#include "ProfileManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <mutex>

ProfileManager::ProfileManager(const std::string &storagePath) : path(storagePath), nextId(1) {
    load();
}

ProfileManager::~ProfileManager() {
    save();
}

void ProfileManager::rebuildIndex() {
    idToIndex.clear();
    for (size_t i = 0; i < users.size(); ++i) idToIndex[users[i].id] = i;
}

User ProfileManager::createUser(const std::string &name, int age, const std::string &bio, const std::string &contact, const std::string &photo) {
    std::lock_guard<std::mutex> lk(mtx);
    User u(nextId++, name, age, bio, contact, photo);
    users.push_back(u);
    rebuildIndex();
    save();
    return users.back();
}

std::vector<User> ProfileManager::listUsers() const {
    std::lock_guard<std::mutex> lk(mtx);
    return users;
}

bool ProfileManager::save() {
    std::lock_guard<std::mutex> lk(mtx);
    std::ofstream ofs(path, std::ios::trunc);
    if (!ofs) return false;
    for (const auto &u : users) ofs << u.serialize() << "\n";
    return true;
}

bool ProfileManager::load() {
    std::lock_guard<std::mutex> lk(mtx);
    users.clear();
    std::ifstream ifs(path);
    if (!ifs) return false;
    std::string line;
    int maxId = 0;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        User u = User::deserialize(line);
        if (u.id > maxId) maxId = u.id;
        users.push_back(u);
    }
    nextId = maxId + 1;
    rebuildIndex();
    return true;
}

std::unique_lock<std::mutex> ProfileManager::getLock() {
    return std::unique_lock<std::mutex>(mtx);
}

User* ProfileManager::getUserPtrLocked(int id) {
    auto it = idToIndex.find(id);
    if (it == idToIndex.end()) return nullptr;
    return &users[it->second];
}

int ProfileManager::totalUsers() const { std::lock_guard<std::mutex> lk(mtx); return (int)users.size(); }
