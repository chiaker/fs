#include "User.h"
#include <sstream>
#include <algorithm>

User::User() : id(0), age(0) {}

User::User(int id_, const std::string &name_, int age_, const std::string &bio_, const std::string &contact_, const std::string &photo_)
    : id(id_), name(name_), age(age_), bio(bio_), contact(contact_), photo(photo_) {}

static std::string joinInts(const std::vector<int> &v) {
    std::ostringstream oss;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) oss << ',';
        oss << v[i];
    }
    return oss.str();
}

static std::vector<int> splitInts(const std::string &s) {
    std::vector<int> out;
    std::istringstream iss(s);
    std::string token;
    while (std::getline(iss, token, ',')) {
        if (token.empty()) continue;
        out.push_back(std::stoi(token));
    }
    return out;
}

std::string User::serialize() const {
    // simple pipe-separated format: id|name|age|bio|contact|likes|matches
    std::ostringstream oss;
    // new format adds photo between contact and likes: id|name|age|bio|contact|photo|likes|matches
    oss << id << '|' << name << '|' << age << '|' << bio << '|' << contact << '|' << photo << '|' << joinInts(likes) << '|' << joinInts(matches);
    return oss.str();
}

User User::deserialize(const std::string &line) {
    std::istringstream iss(line);
    std::string part;
    std::vector<std::string> parts;
    while (std::getline(iss, part, '|')) parts.push_back(part);

    User u;
    // expect at least 6 parts now (id,name,age,bio,contact,photo)
    if (parts.size() >= 6) {
        u.id = std::stoi(parts[0]);
        u.name = parts[1];
        u.age = std::stoi(parts[2]);
        u.bio = parts[3];
        u.contact = parts[4];
        u.photo = parts[5];
        if (parts.size() > 6) u.likes = splitInts(parts[6]);
        if (parts.size() > 7) u.matches = splitInts(parts[7]);
    }
    return u;
}
