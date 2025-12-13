(function(){
  const getAPIBase = () => {
    const hostname = window.location.hostname || 'localhost';
    const port = window.location.port || '8080';
    return `http://${hostname}:${port}`;
  };
  const API = getAPIBase();

  const getImageUrl = (photoPath) => {
    if (!photoPath) return null;
    if (photoPath.startsWith('http://') || photoPath.startsWith('https://')) {
      return photoPath;
    }
    const path = photoPath.startsWith('/') ? photoPath : `/${photoPath}`;
    return `${API}${path}`;
  };
  const userIdInput = document.getElementById('userId');
  const btn = document.getElementById('btnLoad');
  const area = document.getElementById('matchesArea');

  // Prefer localStorage value if present
  const saved = localStorage.getItem('currentUserId');
  if (saved) userIdInput.value = saved;

  // Also read query parameter ?id= and prefer it
  const searchParams = new URLSearchParams(window.location.search);
  if (searchParams.has('id')) {
    const qid = searchParams.get('id');
    if (qid) userIdInput.value = qid;
  }

  btn.addEventListener('click', async () => {
    const id = userIdInput.value.trim();
    if (!id) {
      alert('Введите ваш ID');
      return;
    }
    localStorage.setItem('currentUserId', id);
    area.innerHTML = '<div class="text-center text-white"><div class="loading"></div><p class="mt-3">Загрузка...</p></div>';
    
    try {
      const res = await fetch(`${API}/matches?id=${encodeURIComponent(id)}`);
      if (!res.ok) throw new Error(res.status + ' ' + res.statusText);
      const arr = await res.json();
      
      if (!Array.isArray(arr) || arr.length === 0) {
        area.innerHTML = '<div class="text-center text-white"><h3>Пока нет мэтчей</h3><p>Продолжайте лайкать профили!</p></div>';
        return;
      }
      
      const html = arr.map(m => {
        const imageUrl = getImageUrl(m.photo);
        const photoHtml = imageUrl
          ? `<img src="${imageUrl}" alt="${m.name}">`
          : '<div style="width:100px;height:100px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:2rem;">👤</div>';
        
        return `
          <div class="match-card">
            ${photoHtml}
            <div class="match-info">
              <h5>${m.name}</h5>
              <p><strong>Email:</strong> ${m.contact}</p>
            </div>
          </div>
        `;
      }).join('\n');
      
      area.innerHTML = html;
      // Attach image error handlers to all images inside matches area
      area.querySelectorAll('img').forEach(img => {
        img.addEventListener('error', () => {
          const placeholder = document.createElement('div');
          placeholder.style.width = '100px';
          placeholder.style.height = '100px';
          placeholder.style.background = 'linear-gradient(135deg,#667eea 0%,#764ba2 100%)';
          placeholder.style.borderRadius = '50%';
          placeholder.style.display = 'flex';
          placeholder.style.alignItems = 'center';
          placeholder.style.justifyContent = 'center';
          placeholder.style.fontSize = '2rem';
          placeholder.innerText = '👤';
          img.replaceWith(placeholder);
        });
        img.addEventListener('error', () => console.warn('Match image failed to load:', img.src));
      });
    } catch (err) {
      area.innerHTML = `<div class="alert alert-danger">Ошибка: ${err.message}</div>`;
    }
  });

  // Auto-load if we have an id
  if (userIdInput.value && userIdInput.value.trim() !== '') {
    setTimeout(() => btn.click(), 100);
  }
})();