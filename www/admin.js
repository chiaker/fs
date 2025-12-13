 (function(){
  const getAPIBase = () => {
    const hostname = window.location.hostname;
    const port = window.location.port || '8080';
    return `http://${hostname}:${port}`;
  };
  const API = getAPIBase();
  const form = document.getElementById('adminForm');
  const result = document.getElementById('result');
  form.addEventListener('submit', async (e)=>{
    e.preventDefault();
    const fileInput = form.querySelector('input[name="photo"]');
    const file = fileInput && fileInput.files && fileInput.files[0];
    const base = new URLSearchParams(new FormData(form));
    const send = async (photoData) => {
      if (photoData) base.set('photo_data', photoData);
      try {
        const res = await fetch(`${API}/admin/add`, { method: 'POST', body: base });
        const txt = await res.text();
        result.innerText = txt;
        form.reset();
      } catch (err) {
        result.innerText = 'error: ' + err.message;
      }
    };
    if (file) {
      const reader = new FileReader();
      reader.onload = () => send(reader.result);
      reader.onerror = () => send(null);
      reader.readAsDataURL(file);
    } else send(null);
  });
})();
