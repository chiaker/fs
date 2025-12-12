const API = 'http://localhost:8080';

let currentUserId = null;
let allUsers = [];
let browseIndex = 0;
let currentCard = null;
let cardStack = [];
let startX = 0;
let startY = 0;
let isDragging = false;

// Initialize when DOM is ready
document.addEventListener('DOMContentLoaded', function() {
  // Restore from localStorage if available
if (localStorage.getItem('currentUserId')) {
  currentUserId = localStorage.getItem('currentUserId');
    loadUsers().then(() => {
    const me = allUsers.find(u => u.id == currentUserId);
      if (me) {
        hideProfileModal();
        showMainContent();
        const startBtn = document.getElementById('startBrowse');
        if (startBtn) startBtn.disabled = false;
      const mmBtn = document.getElementById('myMatches');
      if (mmBtn) mmBtn.disabled = false;
      } else {
        // User ID exists but user not found, show modal
        showProfileModal();
      }
    }).catch((err) => {
      console.error('Error loading users:', err);
      showProfileModal();
    });
  } else {
    showProfileModal();
  }
  
  // Initialize form handler
  const createForm = document.getElementById('createForm');
  if (createForm) {
    console.log('Form found, attaching submit handler');
    createForm.addEventListener('submit', handleCreateProfile);
  } else {
    console.error('Form with id "createForm" not found!');
  }
  
  // Initialize buttons
  const myMatchesBtn = document.getElementById('myMatches');
  if (myMatchesBtn) {
    myMatchesBtn.addEventListener('click', () => {
      if (!currentUserId) {
        alert('Создайте профиль сначала');
        return;
      }
      window.location.href = `matches.html?id=${encodeURIComponent(currentUserId)}`;
    });
  }
  
  const startBrowseBtn = document.getElementById('startBrowse');
  if (startBrowseBtn) {
    startBrowseBtn.addEventListener('click', async () => {
      if (!currentUserId) {
        alert('Создайте профиль сначала');
        return;
      }
      browseIndex = 0;
      await loadUsers();
      showNextCard();
    });
  }
  
  // Action buttons
  const btnLike = document.getElementById('btnLike');
  if (btnLike) {
    btnLike.addEventListener('click', () => {
      if (currentCard) {
        const userId = parseInt(currentCard.dataset.userId);
        handleLike(userId);
        removeCard(currentCard);
      }
    });
  }
  
  const btnDislike = document.getElementById('btnDislike');
  if (btnDislike) {
    btnDislike.addEventListener('click', () => {
      if (currentCard) {
        const userId = parseInt(currentCard.dataset.userId);
        handleDislike(userId);
        removeCard(currentCard);
      }
    });
  }
  
  const btnSuperLike = document.getElementById('btnSuperLike');
  if (btnSuperLike) {
    btnSuperLike.addEventListener('click', () => {
      if (currentCard) {
        const userId = parseInt(currentCard.dataset.userId);
        handleLike(userId); // Super like is same as like for now
        removeCard(currentCard);
      }
    });
  }
});

async function handleCreateProfile(e) {
  e.preventDefault();
  console.log('Form submit triggered');
  
  const form = e.target;
  const msgEl = document.getElementById('createMsg');
  
  // Show loading message and disable button
  const submitBtn = form.querySelector('button[type="submit"]');
  if (submitBtn) {
    submitBtn.disabled = true;
    submitBtn.innerText = 'Создание...';
  }
  
  if (msgEl) {
    msgEl.innerText = 'Создание профиля...';
    msgEl.className = 'mt-3 text-center text-info';
  }
  
  // Validate required fields
  const nameInput = form.querySelector('input[name="name"]');
  const contactInput = form.querySelector('input[name="contact"]');
  
  if (!nameInput || !nameInput.value.trim()) {
    if (msgEl) {
      msgEl.innerText = 'Ошибка: Имя обязательно для заполнения';
      msgEl.className = 'mt-3 text-center text-danger';
    }
    return;
  }
  
  if (!contactInput || !contactInput.value.trim()) {
    if (msgEl) {
      msgEl.innerText = 'Ошибка: Email обязателен для заполнения';
      msgEl.className = 'mt-3 text-center text-danger';
    }
    return;
  }
  
  // Basic email validation
  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
  if (!emailRegex.test(contactInput.value.trim())) {
    if (msgEl) {
      msgEl.innerText = 'Ошибка: Введите корректный email адрес';
      msgEl.className = 'mt-3 text-center text-danger';
    }
    return;
  }
  
  const fileInput = form.querySelector('input[name="photo"]');
  const file = fileInput && fileInput.files && fileInput.files[0];
  const base = new URLSearchParams(new FormData(form));
  
  const sendCreate = async (photoData) => {
    if (photoData) base.set('photo_data', photoData);
    try {
      console.log('Sending request to:', `${API}/create`);
      
      // Add timeout
      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 10000); // 10 second timeout
      
      const res = await fetch(`${API}/create`, {
        method: 'POST',
        body: base,
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        signal: controller.signal
      });
      
      clearTimeout(timeoutId);
      
      if (!res.ok) {
        throw new Error(`HTTP ${res.status}: ${res.statusText}`);
      }
      
      const txt = await res.text();
      console.log('Server response:', txt);
      
      if (msgEl) {
        msgEl.innerText = txt;
        msgEl.className = 'mt-3 text-center text-info';
      }
      
      const m = txt.match(/created id=(\d+)/);
      if (m) {
        currentUserId = m[1];
        localStorage.setItem('currentUserId', currentUserId);
        if (msgEl) {
          msgEl.innerText = `Профиль создан! Ваш ID: ${currentUserId}`;
          msgEl.className = 'mt-3 text-center text-success';
        }
        await loadUsers();
        hideProfileModal();
        showMainContent();
        const startBtn = document.getElementById('startBrowse');
        if (startBtn) startBtn.disabled = false;
        const mmBtn = document.getElementById('myMatches');
        if (mmBtn) mmBtn.disabled = false;
        form.reset();
        setTimeout(() => {
          const startBtn = document.getElementById('startBrowse');
          if (startBtn) startBtn.click();
        }, 500);
      } else {
        // Server returned error message
        if (msgEl) {
          msgEl.innerText = 'Ошибка: ' + txt;
          msgEl.className = 'mt-3 text-center text-danger';
        }
        if (submitBtn) {
          submitBtn.disabled = false;
          submitBtn.innerText = 'Создать профиль';
        }
      }
    } catch (err) {
      console.error('Error creating profile:', err);
      if (msgEl) {
        let errorMsg = 'Ошибка подключения: ' + err.message;
        if (err.name === 'AbortError') {
          errorMsg = 'Превышено время ожидания. Убедитесь, что сервер запущен на порту 8080.';
        } else if (err.message.includes('Failed to fetch') || err.message.includes('NetworkError')) {
          errorMsg = 'Не удалось подключиться к серверу. Убедитесь, что сервер запущен: .\\dating_server.exe';
        }
        msgEl.innerText = errorMsg;
        msgEl.className = 'mt-3 text-center text-danger';
      }
      if (submitBtn) {
        submitBtn.disabled = false;
        submitBtn.innerText = 'Создать профиль';
      }
    }
  };
  
  if (file) {
    const reader = new FileReader();
    reader.onload = () => {
      console.log('Photo loaded, sending...');
      sendCreate(reader.result);
    };
    reader.onerror = () => {
      console.warn('Photo read error, sending without photo');
      sendCreate(null);
    };
    reader.readAsDataURL(file);
  } else {
    console.log('No photo, sending without photo');
    sendCreate(null);
  }
}

function showProfileModal() {
  const modal = document.getElementById('profileModal');
  if (modal) modal.classList.remove('hidden');
}

function hideProfileModal() {
  const modal = document.getElementById('profileModal');
  if (modal) modal.classList.add('hidden');
}

function showMainContent() {
  const main = document.getElementById('mainContent');
  if (main) main.style.display = 'block';
}

function createTinderCard(user) {
  const card = document.createElement('div');
  card.className = 'tinder-card';
  card.dataset.userId = user.id;
  
  const imageHtml = user.photo 
    ? `<img src="${API}${user.photo}" alt="${user.name}">`
    : `<div class="card-image-placeholder">👤</div>`;
  
  card.innerHTML = `
    <div class="card-image-container">
      ${imageHtml}
    </div>
    <div class="card-info">
      <div class="card-name-age">${user.name}, ${user.age || '?'}</div>
      <div class="card-bio">${user.bio || 'Нет описания'}</div>
    </div>
  `;
  
  // Add swipe handlers
  addSwipeHandlers(card);
  
  return card;
}

function addSwipeHandlers(card) {
  let startX = 0, startY = 0, currentX = 0, currentY = 0;
  let isDown = false;
  
  card.addEventListener('mousedown', (e) => {
    isDown = true;
    startX = e.clientX;
    startY = e.clientY;
    card.style.transition = 'none';
  });
  
  card.addEventListener('mousemove', (e) => {
    if (!isDown) return;
    e.preventDefault();
    currentX = e.clientX - startX;
    currentY = e.clientY - startY;
    
    const rotate = currentX * 0.1;
    card.style.transform = `translate(${currentX}px, ${currentY}px) rotate(${rotate}deg)`;
    
    if (currentX > 50) {
      card.classList.add('swiping-right');
      card.classList.remove('swiping-left');
    } else if (currentX < -50) {
      card.classList.add('swiping-left');
      card.classList.remove('swiping-right');
    } else {
      card.classList.remove('swiping-right', 'swiping-left');
    }
  });
  
  card.addEventListener('mouseup', () => {
    if (!isDown) return;
    isDown = false;
    card.style.transition = 'transform 0.3s ease';
    
    if (Math.abs(currentX) > 100) {
      if (currentX > 0) {
        handleLike(parseInt(card.dataset.userId));
      } else {
        handleDislike(parseInt(card.dataset.userId));
      }
      removeCard(card);
    } else {
      card.style.transform = '';
      card.classList.remove('swiping-right', 'swiping-left');
    }
  });
  
  card.addEventListener('mouseleave', () => {
    if (isDown) {
      isDown = false;
      card.style.transition = 'transform 0.3s ease';
      if (Math.abs(currentX) > 100) {
        if (currentX > 0) {
          handleLike(parseInt(card.dataset.userId));
        } else {
          handleDislike(parseInt(card.dataset.userId));
        }
        removeCard(card);
      } else {
        card.style.transform = '';
        card.classList.remove('swiping-right', 'swiping-left');
      }
    }
  });
  
  // Touch events for mobile
  card.addEventListener('touchstart', (e) => {
    isDown = true;
    startX = e.touches[0].clientX;
    startY = e.touches[0].clientY;
    card.style.transition = 'none';
  });
  
  card.addEventListener('touchmove', (e) => {
    if (!isDown) return;
    e.preventDefault();
    currentX = e.touches[0].clientX - startX;
    currentY = e.touches[0].clientY - startY;
    
    const rotate = currentX * 0.1;
    card.style.transform = `translate(${currentX}px, ${currentY}px) rotate(${rotate}deg)`;
    
    if (currentX > 50) {
      card.classList.add('swiping-right');
      card.classList.remove('swiping-left');
    } else if (currentX < -50) {
      card.classList.add('swiping-left');
      card.classList.remove('swiping-right');
    } else {
      card.classList.remove('swiping-right', 'swiping-left');
    }
  });
  
  card.addEventListener('touchend', () => {
    if (!isDown) return;
    isDown = false;
    card.style.transition = 'transform 0.3s ease';
    
    if (Math.abs(currentX) > 100) {
      if (currentX > 0) {
        handleLike(parseInt(card.dataset.userId));
      } else {
        handleDislike(parseInt(card.dataset.userId));
      }
      removeCard(card);
    } else {
      card.style.transform = '';
      card.classList.remove('swiping-right', 'swiping-left');
    }
  });
}

function removeCard(card) {
  card.classList.add('removing');
  setTimeout(() => {
    if (card.parentNode) {
      card.parentNode.removeChild(card);
    }
    showNextCard();
  }, 300);
}

function showNextCard() {
  const area = document.getElementById('cardArea');
  if (!currentUserId) {
    area.innerHTML = '<div class="welcome-message"><h2>Создайте профиль</h2><p>Чтобы начать просмотр</p></div>';
    return;
  }
  
  // Find next candidate
  let found = null;
  let attempts = 0;
  while (attempts < allUsers.length) {
    const u = allUsers[browseIndex % allUsers.length];
    if (u.id !== parseInt(currentUserId)) {
      found = u;
      browseIndex = (browseIndex + 1) % allUsers.length;
      break;
    }
    browseIndex = (browseIndex + 1) % allUsers.length;
    attempts++;
  }
  
  if (!found) {
    area.innerHTML = '<div class="empty-state"><h3>Больше нет профилей</h3><p>Попробуйте позже</p></div>';
    return;
  }
  
  const card = createTinderCard(found);
  area.innerHTML = '';
  area.appendChild(card);
  currentCard = card;
}

async function handleLike(userId) {
  if (!currentUserId) return;
  try {
    const res = await fetch(`${API}/like?user=${currentUserId}&target=${userId}`, { method: 'POST' });
    const txt = await res.text();
    if (txt.toLowerCase().includes('match')) {
      showMatchNotification(txt);
    }
  } catch (err) {
    console.error('Error liking:', err);
  }
}

async function handleDislike(userId) {
  // Just skip, no API call needed for now
  console.log('Disliked:', userId);
}

function showMatchNotification(message) {
  const notification = document.createElement('div');
  notification.className = 'match-notification';
  notification.innerHTML = `
    <h2>💕 Это Мэтч!</h2>
    <p>${message}</p>
    <button class="btn btn-primary" onclick="this.parentElement.remove()">Отлично!</button>
  `;
  document.body.appendChild(notification);
  setTimeout(() => notification.classList.add('show'), 10);
}

async function loadUsers() {
  try {
    const res = await fetch(`${API}/users`);
    if (!res.ok) throw new Error(res.status + ' ' + res.statusText);
    const users = await res.json();
    allUsers = users.map(u => ({
      id: u.id,
      name: u.name || '',
      age: u.age || '',
      bio: u.bio || '',
      photo: u.photo || ''
    }));
  } catch (err) {
    allUsers = [];
    console.error('Error loading users:', err);
  }
}
