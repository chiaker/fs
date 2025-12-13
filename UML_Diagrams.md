┌─────────────┐
│    User     │
├─────────────┤
│ +id         │
│ +name       │
│ +age        │
│ +bio        │
│ +contact    │
│ +likes[]    │
│ +matches[]  │
│ +photo      │
└─────────────┘
       ▲
       │ (managed by)
       │
┌─────────────┐         ┌─────────────┐
│ProfileMgr   │────────▶│ MatchMgr    │
├─────────────┤ uses    ├─────────────┤
│ -users[]    │         │ -profiles&  │
│ -idToIndex  │         │ -matchCount │
│ -mtx        │         │ -mtx        │
└─────────────┘         └─────────────┘
       ▲                       ▲
       │                       │
       │ uses                  │ uses
       │                       │
┌─────────────┐                │
│HTTPServer   │────────────────┘
├─────────────┤
│ -port       │
│ -listen_sock│
│ -pm*        │
│ -mm*        │
└─────────────┘