(function(){
  const API = 'http://localhost:8080';
  const userIdInput = document.getElementById('userId');
  const btn = document.getElementById('btnLoad');
  const area = document.getElementById('matchesArea');

  // prefer localStorage value if present
  const saved = localStorage.getItem('currentUserId');
  if (saved) userIdInput.value = saved;

  // also read query parameter ?id= and prefer it
  const searchParams = new URLSearchParams(window.location.search);
  if (searchParams.has('id')) {
    const qid = searchParams.get('id');
    if (qid) userIdInput.value = qid;
  }

  btn.addEventListener('click', async ()=>{
    const id = userIdInput.value.trim();
    if (!id) return alert('Enter your id');
    localStorage.setItem('currentUserId', id);
    area.innerHTML = 'loading...';
    try {
      const res = await fetch(`${API}/matches?id=${encodeURIComponent(id)}`);
      if (!res.ok) throw new Error(res.status + ' ' + res.statusText);
      const arr = await res.json();
      if (!Array.isArray(arr) || arr.length===0) { area.innerHTML = '<div class="text-muted">No matches yet</div>'; return; }
      const html = arr.map(m=>{
        return `<div class="card mb-2"><div class="card-body d-flex gap-3 align-items-center">${m.photo?`<img src="${m.photo}" style="max-width:120px" class="rounded">`:''}<div><h5 class="mb-0">${m.name}</h5><div class="small text-muted">${m.contact}</div></div></div></div>`;
      }).join('\n');
      area.innerHTML = html;
    } catch (err) {
      area.innerText = 'error: ' + err.message;
    }
  });

  // If we have an id in the input pre-filled, auto-load matches
  if (userIdInput.value && userIdInput.value.trim() !== '') {
    btn.click();
  }
})();
