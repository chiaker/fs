#pragma once
#include <string>
#include <vector>

class User {
public:
    int id;
    std::string name;
    int age;
    std::string bio;
    std::string contact;
    std::vector<int> likes; // ids of users this user liked
    std::vector<int> matches; // ids of matched users
    std::vector<int> superlikes; // ids of users this user superliked
    std::string photo; // relative path to photo under www/uploads

    User();
    User(int id_, const std::string &name_, int age_, const std::string &bio_, const std::string &contact_, const std::string &photo_ = "");

    std::string serialize() const; // for file storage
    static User deserialize(const std::string &line);
};
