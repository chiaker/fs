#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <map>
#include <fstream>
#include <thread>
#include <cstdio>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket close
#endif

#include "ProfileManager.h"
#include "MatchManager.h"
#include <filesystem>

using namespace std;

static string urlDecode(const string &s) {
    string out;
    out.reserve(s.size());
    for (size_t i=0;i<s.size();++i) {
        char c = s[i];
        if (c == '+') out.push_back(' ');
        else if (c == '%' && i+2 < s.size()) {
            string hex = s.substr(i+1,2);
            char decoded = (char) strtol(hex.c_str(), nullptr, 16);
            out.push_back(decoded);
            i += 2;
        } else out.push_back(c);
    }
    return out;
}

static inline bool startsWith(const string &s, const string &prefix) { return s.rfind(prefix, 0) == 0; }
static inline bool ends_with(const string &s, const string &suffix) {
    if (s.size() < suffix.size()) return false;
    return s.compare(s.size()-suffix.size(), suffix.size(), suffix) == 0;
}

// Base64 decode
static std::string base64_decode(const std::string &in) {
    std::string out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[(unsigned)"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;
    int val=0, valb=-8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val<<6) + T[c];
        valb += 6;
        if (valb>=0) { out.push_back(char((val>>valb)&0xFF)); valb -= 8; }
    }
    return out;
}

static string saveImageFromDataUrl(const string &dataurl) {
    // dataurl: data:image/png;base64,<b64>
    auto pos = dataurl.find("base64,");
    string b64 = dataurl;
    string ext = ".bin";
    if (pos != string::npos) {
        string header = dataurl.substr(0, pos);
        b64 = dataurl.substr(pos + 7);
        if (header.find("image/png") != string::npos) ext = ".png";
        else if (header.find("image/jpeg") != string::npos) ext = ".jpg";
        else if (header.find("image/gif") != string::npos) ext = ".gif";
    } else {
        // maybe a plain base64 without header, keep .bin
    }
    if (b64.empty()) return "";
    string bytes = base64_decode(b64);
    if (bytes.empty()) return "";
    // generate filename
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::ostringstream name;
    name << "img_" << ms << ext;
    string filename = name.str();
    string path = string("www/uploads/") + filename;
    // ensure directory exists
    std::filesystem::create_directories("www/uploads");
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return "";
    ofs.write(bytes.data(), bytes.size());
    return filename; // store only filename; frontend will request /uploads/<filename>
}

static map<string,string> parseQuery(const string &q) {
    map<string,string> m;
    size_t pos = 0;
    while (pos < q.size()) {
        auto amp = q.find('&', pos);
        string part = q.substr(pos, amp==string::npos ? string::npos : amp-pos);
        auto eq = part.find('=');
        if (eq != string::npos) {
            string k = urlDecode(part.substr(0, eq));
            string v = urlDecode(part.substr(eq+1));
            m[k]=v;
        } else if (!part.empty()) {
            m[urlDecode(part)] = "";
        }
        if (amp==string::npos) break;
        pos = amp+1;
    }
    return m;
}

static map<string,string> parseForm(const string &body) {
    return parseQuery(body);
}

static string escapeJson(const string &s) {
    string out; out.reserve(s.size()*2);
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

static void sendResponse(int client_sock, const string &status, const string &body, const string &contentType="text/plain; charset=utf-8") {
    ostringstream resp;
    resp << "HTTP/1.1 " << status << "\r\n";
    resp << "Access-Control-Allow-Origin: *\r\n";
    resp << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    resp << "Access-Control-Allow-Headers: Content-Type\r\n";
    resp << "Content-Type: " << contentType << "\r\n";
    resp << "Content-Length: " << body.size() << "\r\n";
    resp << "Connection: close\r\n";
    resp << "\r\n";
    resp << body;
    string respStr = resp.str();
    send(client_sock, respStr.c_str(), (int)respStr.size(), 0);
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        return 1;
    }
#endif

    ProfileManager pm("data/users.db");
    pm.load();
    MatchManager mm(pm);

    int port = 8080;
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) { cerr << "socket() failed" << endl; return 1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    if (bind(listen_sock, (sockaddr*)&addr, sizeof(addr)) < 0) { cerr << "bind failed" << endl; return 1; }
    if (listen(listen_sock, 10) < 0) { cerr << "listen failed" << endl; return 1; }

    // make sure uploads and data dirs exist
    std::filesystem::create_directories("www/uploads");
    std::filesystem::create_directories("data");
    // ensure users.db exists (create empty file if missing)
    std::string users_db_path = "data/users.db";
    if (!std::filesystem::exists(users_db_path)) {
        std::ofstream ofs(users_db_path, std::ios::app);
        // leave empty and close
    }
    cout << "Server listening on port " << port << "\n";

    while (true) {
        sockaddr_in client{};
        socklen_t clientlen = sizeof(client);
        int client_sock = accept(listen_sock, (sockaddr*)&client, &clientlen);
        if (client_sock < 0) { cerr << "accept failed" << endl; continue; }

        // spawn a thread to handle the client
        std::thread([client_sock, &pm, &mm]() {
            // read request headers first
            string req;
            {
                char buf[8192];
                int received = recv(client_sock, buf, sizeof(buf)-1, 0);
                if (received <= 0) { closesocket(client_sock); return; }
                buf[received] = '\0';
                req = string(buf, received);
            }

            // parse request line
            istringstream rstream(req);
            string method, fullpath, version;
            rstream >> method >> fullpath >> version;
            string path = fullpath;
            string query;
            auto qpos = fullpath.find('?');
            if (qpos != string::npos) { path = fullpath.substr(0, qpos); query = fullpath.substr(qpos+1); }

            // Find header end and extract body
            string body;
            auto hdr_end = req.find("\r\n\r\n");
            if (hdr_end != string::npos) {
                body = req.substr(hdr_end + 4);
                
                // Check Content-Length header to read full body if needed
                auto cl_pos = req.find("Content-Length:");
                if (cl_pos != string::npos) {
                    auto cl_end = req.find("\r\n", cl_pos);
                    if (cl_end != string::npos) {
                        string cl_str = req.substr(cl_pos + 15, cl_end - cl_pos - 15);
                        // trim whitespace
                        while (!cl_str.empty() && (cl_str[0] == ' ' || cl_str[0] == '\t')) cl_str = cl_str.substr(1);
                        try {
                            int content_length = stoi(cl_str);
                            int body_received = (int)body.size();
                            // If body is incomplete, read more
                            if (body_received < content_length) {
                                int remaining = content_length - body_received;
                                char* extra_buf = new char[remaining + 1];
                                int extra_received = recv(client_sock, extra_buf, remaining, 0);
                                if (extra_received > 0) {
                                    body += string(extra_buf, extra_received);
                                }
                                delete[] extra_buf;
                            }
                        } catch (...) {
                            // ignore parse error, use what we have
                        }
                    }
                }
            }

            // handle OPTIONS quickly
            if (method == "OPTIONS") {
                sendResponse(client_sock, "200 OK", "", "text/plain");
                closesocket(client_sock);
                return;
            }

            string response_body;
            string status = "200 OK";
            string contentType = "text/plain; charset=utf-8";

            try {
                // serve uploaded files from www/uploads
                if (method == "GET" && startsWith(path, "/uploads/")) {
                    string local = string("www") + path;
                    std::ifstream ifs(local, std::ios::binary);
                    if (!ifs) { status = "404 Not Found"; response_body = "file not found"; }
                    else {
                        std::ostringstream oss; oss << ifs.rdbuf();
                        // determine content type
                        string contentTypeLocal = "application/octet-stream";
                        if (ends_with(path, ".png") || ends_with(path, ".PNG")) contentTypeLocal = "image/png";
                        else if (ends_with(path, ".jpg") || ends_with(path, ".jpeg") || ends_with(path, ".JPG")) contentTypeLocal = "image/jpeg";
                        else if (ends_with(path, ".gif")) contentTypeLocal = "image/gif";
                        response_body = oss.str(); contentType = contentTypeLocal;
                    }
                }

                // Serve static files
                if (method == "GET" && (path == "/" || path == "/index.html")) {
                    std::ifstream ifs("www/index.html");
                    if (ifs) {
                        ostringstream oss; oss << ifs.rdbuf();
                        response_body = oss.str(); contentType = "text/html; charset=utf-8";
                    } else { status = "404 Not Found"; response_body = "index.html not found"; }
                }
                else if (method == "GET" && path == "/matches.html") {
                    std::ifstream ifs("www/matches.html");
                    if (ifs) {
                        ostringstream oss; oss << ifs.rdbuf();
                        response_body = oss.str(); contentType = "text/html; charset=utf-8";
                    } else { status = "404 Not Found"; response_body = "matches.html not found"; }
                }
                else if (method == "GET" && path == "/app.js") {
                    std::ifstream ifs("www/app.js");
                    if (ifs) { ostringstream oss; oss << ifs.rdbuf(); response_body = oss.str(); contentType = "application/javascript; charset=utf-8"; }
                    else { status = "404 Not Found"; response_body = "app.js not found"; }
                }
                else if (method == "GET" && path == "/matches.js") {
                    std::ifstream ifs("www/matches.js");
                    if (ifs) { ostringstream oss; oss << ifs.rdbuf(); response_body = oss.str(); contentType = "application/javascript; charset=utf-8"; }
                    else { status = "404 Not Found"; response_body = "matches.js not found"; }
                }
                else if (method == "GET" && path == "/styles.css") {
                    std::ifstream ifs("www/styles.css");
                    if (ifs) { ostringstream oss; oss << ifs.rdbuf(); response_body = oss.str(); contentType = "text/css; charset=utf-8"; }
                    else { status = "404 Not Found"; response_body = "styles.css not found"; }
                }
                else if (method == "GET" && path == "/admin") {
                    std::ifstream ifs("www/admin.html");
                    if (ifs) {
                        ostringstream oss; oss << ifs.rdbuf();
                        response_body = oss.str(); contentType = "text/html; charset=utf-8";
                    } else { status = "404 Not Found"; response_body = "admin page not found"; }
                }
                else if (method == "GET" && path == "/admin.js") {
                    std::ifstream ifs("www/admin.js");
                    if (ifs) { ostringstream oss; oss << ifs.rdbuf(); response_body = oss.str(); contentType = "application/javascript; charset=utf-8"; }
                    else { status = "404 Not Found"; response_body = "admin.js not found"; }
                }
                else if (method == "GET" && path == "/users") {
                    auto users = pm.listUsers();
                    ostringstream oss;
                    oss << "[";
                    for (size_t i=0;i<users.size();++i) {
                        const auto &u = users[i];
                        if (i) oss << ",";
                        string photoUrl = "";
                        if (!u.photo.empty()) {
                            // If it's a URL, use it directly
                            if (u.photo.rfind("http://", 0) == 0 || u.photo.rfind("https://", 0) == 0) {
                                photoUrl = u.photo;
                            } else {
                                // Extract basename from any possible path (www/uploads/, uploads/, /uploads/, or just filename)
                                string fname = u.photo;
                                auto pos = fname.find_last_of("/\\");
                                if (pos != string::npos) fname = fname.substr(pos+1);
                                photoUrl = string("/uploads/") + fname;
                            }
                        }
                        oss << "{\"id\":"<<u.id<<",\"name\":\""<<escapeJson(u.name)<<"\",\"age\":"<<u.age<<",\"bio\":\""<<escapeJson(u.bio)<<"\",\"photo\":\""<<escapeJson(photoUrl)<<"\"}";
                    }
                    oss << "]";
                    response_body = oss.str(); contentType = "application/json; charset=utf-8";
                }
                else if (method == "POST" && path == "/admin/add") {
                    auto form = parseForm(body);
                    string name = form.count("name")?form["name"]:"";
                    int age = 0; if (form.count("age") && !form["age"].empty()) { try { age = stoi(form["age"]); } catch(...) { age = 0; } }
                    string bio = form.count("bio")?form["bio"]:"";
                    string contact = form.count("contact")?form["contact"]:"";
                    string photoFile = "";
                    if (form.count("photo_data") && !form["photo_data"].empty()) {
                        string fname = saveImageFromDataUrl(form["photo_data"]);
                        if (!fname.empty()) photoFile = fname;
                    }
                    if (name.empty() || contact.empty()) { status = "400 Bad Request"; response_body = "name and contact required"; }
                    else { User u = pm.createUser(name, age, bio, contact, photoFile); response_body = string("created id=") + to_string(u.id) + "\n"; }
                }
                else if (method == "POST" && path == "/create") {
                    auto form = parseForm(body);
                    string name = form.count("name")?form["name"]:"";
                    int age = 0; if (form.count("age") && !form["age"].empty()) { try { age = stoi(form["age"]); } catch(...) { age = 0; } }
                    string bio = form.count("bio")?form["bio"]:"";
                    string contact = form.count("contact")?form["contact"]:"";
                    string photoFile = "";
                    if (form.count("photo_data") && !form["photo_data"].empty()) {
                        string fname = saveImageFromDataUrl(form["photo_data"]);
                        if (!fname.empty()) photoFile = fname;
                    }
                    if (name.empty() || contact.empty()) { status = "400 Bad Request"; response_body = "name and contact required"; }
                    else { User u = pm.createUser(name, age, bio, contact, photoFile); response_body = string("created id=") + to_string(u.id) + "\n"; }
                }
                else if (method == "POST" && path == "/like") {
                    auto q = parseQuery(query);
                    if (!q.count("user") || !q.count("target")) { status = "400 Bad Request"; response_body = "user and target query params required"; }
                    else {
                        int userId = 0, targetId = 0;
                        try {
                            if (q["user"].empty() || q["target"].empty()) throw std::invalid_argument("empty");
                            userId = stoi(q["user"]); targetId = stoi(q["target"]);
                        } catch (...) { status = "400 Bad Request"; response_body = "invalid user or target id"; }
                        if (status == "200 OK") {
                            bool isSuper = false;
                            if (q.count("super") && !q["super"].empty()) {
                                string v = q["super"];
                                if (v == "1" || v == "true" || v == "True") isSuper = true;
                            }
                            string msg; bool isMatch = mm.like(userId, targetId, msg, isSuper);
                            response_body = msg + "\n";
                        }
                    }
                }
                else if (method == "GET" && path == "/matches") {
                    auto q = parseQuery(query);
                    if (!q.count("id")) { status = "400 Bad Request"; response_body = "id query param required"; }
                    else {
                        int id = 0;
                        try { if (q["id"].empty()) throw std::invalid_argument("empty"); id = stoi(q["id"]); }
                        catch (...) { status = "400 Bad Request"; response_body = "invalid id"; }
                        if (status == "200 OK") {
                            auto lock = pm.getLock();
                            User* u = pm.getUserPtrLocked(id);
                            if (!u) { status = "404 Not Found"; response_body = "user not found"; }
                            else {
                                // return JSON array of matches
                                ostringstream oss; oss << "[";
                                for (size_t i=0;i<u->matches.size();++i) {
                                    int m = u->matches[i];
                                    User* other = pm.getUserPtrLocked(m);
                                    if (!other) continue;
                                    if (i) oss << ",";
                                    string photoUrl = "";
                                    if (!other->photo.empty()) {
                                        if (other->photo.rfind("http://", 0) == 0 || other->photo.rfind("https://", 0) == 0) {
                                            photoUrl = other->photo;
                                        } else {
                                            string fname = other->photo;
                                            auto pos2 = fname.find_last_of("/\\");
                                            if (pos2 != string::npos) fname = fname.substr(pos2+1);
                                            photoUrl = string("/uploads/") + fname;
                                        }
                                    }
                                    oss << "{";
                                    oss << "\"id\":" << other->id << ",";
                                    oss << "\"name\":\"" << escapeJson(other->name) << "\",";
                                    oss << "\"contact\":\"" << escapeJson(other->contact) << "\",";
                                    oss << "\"photo\":\"" << escapeJson(photoUrl) << "\"";
                                    oss << "}";
                                }
                                oss << "]";
                                response_body = oss.str(); contentType = "application/json; charset=utf-8";
                            }
                        }
                    }
                }
                else if (method == "GET" && path == "/stats") {
                    ostringstream oss; oss << "total_users=" << pm.totalUsers() << "\n"; oss << "total_matches=" << mm.totalMatches() << "\n"; response_body = oss.str();
                }
                else {
                    status = "404 Not Found"; response_body = "unknown endpoint\n";
                }
            } catch (const std::exception &ex) {
                status = "500 Internal Server Error"; response_body = string("error: ") + ex.what() + "\n";
            }

            sendResponse(client_sock, status, response_body, contentType);
            closesocket(client_sock);
        }).detach();
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
