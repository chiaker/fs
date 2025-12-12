# Simple C++ Dating Web App (native HTTP server)

This project is a minimal native C++ HTTP server for a simple dating app. It implements:

* Basic web endpoints for creating user profiles, listing users, liking profiles.
* Match detection: when two users like each other, contacts are revealed.
* Persistent storage using files (STL `fstream`).
* Uses STL containers and algorithms, and OOP with at least three classes.

Requirements

* Windows (example build instructions for Visual Studio or MinGW)
* Compiler supporting C++11 or later

Build (MinGW / g++)

```powershell
g++ -std=c++17 -O2 -I./src src/*.cpp -o dating_server.exe -lws2_32
```

Build (Visual Studio Developer PowerShell)

```powershell
cl /EHsc /std:c++17 src\\*.cpp ws2_32.lib
```

Run

```powershell
.\\dating_server.exe
```

The server listens on port 8080 by default. Endpoints:

* `GET /users` — list users (simple JSON-like text)
* `POST /create` — create user (body: `name=...&age=...&bio=...&contact=...`)
* `POST /like?user=<id>&target=<id>` — user likes target
* `GET /matches?id=<id>` — list matches and contacts for user
* `GET /stats` — global statistics (total users, total matches)

Data is stored in `data/users.db` relative to the executable.

Notes

* This is a learning example. The HTTP parsing is minimal and not production-ready.
* Uses only STL (`vector`, `map`, `string`, `fstream`, algorithms) and OOP.
