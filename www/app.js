const API = 'http://localhost:8080';

let currentUserId = null;
let allUsers = [];
let browseIndex = 0;

// restore from localStorage if available
if (localStorage.getItem('currentUserId')) {
  currentUserId = localStorage.getItem('currentUserId');
  // pre-load users and render info
  loadUsers().then(()=>{
    const me = allUsers.find(u => u.id == currentUserId);
    if (me) renderYourInfo(me);
      document.getElementById('startBrowse').disabled = false;
      const mmBtn = document.getElementById('myMatches');
      if (mmBtn) mmBtn.disabled = false;
  }).catch(()=>{});

async function fetchStats() {
  try {
    const res = await fetch(`${API}/stats`);
    if (!res.ok) throw new Error(res.status + ' ' + res.statusText);
    const txt = await res.text();
    document.getElementById('stats').innerText = txt;
  } catch (err) {
    document.getElementById('stats').innerText = 'error: ' + err.message;
  }
}

function renderYourInfo(user) {
  const el = document.getElementById('yourInfo');
  if (!user) { el.innerText = 'No profile yet'; return; }
  const img = user.photo?`<div class="mb-2"><img src="${user.photo}" class="img-fluid rounded" style="max-width:120px"></div>`:'';
  el.innerHTML = `${img}<strong>#${user.id}</strong> ${user.name}, ${user.age}\n<div class="small text-muted">${user.bio}</div>`;
}

function renderCardArea() {
  const area = document.getElementById('cardArea');
  if (!currentUserId) { area.innerHTML = '<div class="text-muted">Create your profile first, then start browsing.</div>'; return; }
  if (allUsers.length === 0) { area.innerHTML = '<div class="text-muted">No other users yet.</div>'; return; }

  // find next candidate not the current user
  let i = browseIndex;
  let found = null;
  for (let attempts = 0; attempts < allUsers.length; ++attempts) {
    const u = allUsers[i % allUsers.length];
    if (u.id !== parseInt(currentUserId)) { found = u; browseIndex = (i+1) % allUsers.length; break; }
    i = (i+1) % allUsers.length;
  }
  if (!found) { area.innerHTML = '<div class="text-muted">No other users to show.</div>'; return; }

  area.innerHTML = `
    <div class="card">
      <div class="card-body">
        <h5 class="card-title">${found.name}, ${found.age}</h5>
        ${found.photo?`<img src="${found.photo}" class="img-fluid mb-2" style="max-width:200px">`:''}
        <p class="card-text">${found.bio}</p>
        <div class="d-flex gap-2">
          <button id="btnLike" class="btn btn-success">Like</button>
          <button id="btnSkip" class="btn btn-secondary">Skip</button>
        </div>
      </div>
    </div>
  `;

  document.getElementById('btnLike').addEventListener('click', async () => {
    if (!currentUserId) { alert('Create your profile first'); return; }
    try {
      const res = await fetch(`${API}/like?user=${currentUserId}&target=${found.id}`, { method: 'POST' });
      const txt = await res.text();
      if (txt.toLowerCase().includes('match')) {
        // server includes contacts in message
        alert('It\'s a match! ' + txt);
      } else {
        // simple feedback
        // proceed to next
      }
    } catch (err) { alert('network error: ' + err.message); }
    renderCardArea();
  });

  document.getElementById('btnSkip').addEventListener('click', () => {
    renderCardArea();
  });
}

async function loadUsers() {
  try {
    const res = await fetch(`${API}/users`);
    if (!res.ok) throw new Error(res.status + ' ' + res.statusText);
    const users = await res.json();
    // server returns array of {id,name,age,bio}
    allUsers = users.map(u => ({ id: u.id, name: u.name || '', age: u.age || '', bio: u.bio || '', photo: u.photo || '' }));
  } catch (err) {
    allUsers = [];
    document.getElementById('cardArea').innerText = 'error loading users: ' + err.message;
  }
}

document.getElementById('createForm').addEventListener('submit', async (e) => {
  e.preventDefault();
  const form = e.target;
  const fileInput = form.querySelector('input[name="photo"]');
  const file = fileInput && fileInput.files && fileInput.files[0];
  const base = new URLSearchParams(new FormData(form));
  const sendCreate = async (photoData) => {
    if (photoData) base.set('photo_data', photoData);
    try {
      const res = await fetch(`${API}/create`, { method: 'POST', body: base, headers: { 'Content-Type': 'application/x-www-form-urlencoded' } });
      const txt = await res.text();
      document.getElementById('createMsg').innerText = txt;
      const m = txt.match(/created id=(\d+)/);
      if (m) {
        currentUserId = m[1];
        localStorage.setItem('currentUserId', currentUserId);
        document.getElementById('createMsg').innerText += ` — your id: ${currentUserId}`;
        await loadUsers();
        const me = allUsers.find(u => u.id == currentUserId);
        renderYourInfo(me);
        document.getElementById('startBrowse').disabled = false;
        const mmBtn = document.getElementById('myMatches');
        if (mmBtn) mmBtn.disabled = false;
        const cf = document.getElementById('createForm'); if (cf) cf.style.display = 'none';
        document.getElementById('startBrowse').click();
        await fetchStats();
      }
      form.reset();
    } catch (err) {
      document.getElementById('createMsg').innerText = 'error: ' + err.message;
    }
  };
  if (file) {
    const reader = new FileReader();
    reader.onload = () => { sendCreate(reader.result); };
    reader.onerror = () => { sendCreate(null); };
    reader.readAsDataURL(file);
  } else {
    sendCreate(null);
  }
});

// My Matches button: navigate to matches page (passes id in query string)
const myMatchesBtn = document.getElementById('myMatches');
if (myMatchesBtn) {
  myMatchesBtn.addEventListener('click', () => {
    if (!currentUserId) { alert('Create your profile first'); return; }
    // navigate and include id param for convenience
    window.location.href = `matches.html?id=${encodeURIComponent(currentUserId)}`;
  });
}

document.getElementById('startBrowse').addEventListener('click', async () => {
  if (!currentUserId) { alert('Create your profile first'); return; }
  browseIndex = 0;
  await loadUsers();
  renderCardArea();
});

// initial load
fetchStats();
}