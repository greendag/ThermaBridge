document.getElementById('prov').addEventListener('submit', async function(e){
  e.preventDefault();
  const form = e.target;
  const data = new URLSearchParams(new FormData(form));
  const status = document.getElementById('status');
  status.textContent = 'Saving...';
  try{
    // Some captive-portal captive browsers (or mobile browsers) block fetch/post due to
    // DNS/HTTPS redirection quirks. Send an explicit content-type header and timeout.
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 8000);
    const res = await fetch('/save', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: data.toString(),
      signal: controller.signal
    });
    clearTimeout(timeout);
    if (!res.ok) {
      status.textContent = 'Server error: ' + res.status;
      return;
    }
    const txt = await res.text();
    status.innerHTML = txt;
  }catch(err){
    // Fallback: some captive portals block fetch; try a plain form submit which
    // is more likely to be allowed by the browser's captive-portal handling.
    console.warn('Fetch failed, falling back to native submit', err);
    status.textContent = 'Fetch failed, falling back to form submit...';
    // Remove event listener and submit normally
    form.removeEventListener('submit', arguments.callee);
    form.submit();
  }
});

// Prefill form fields from /config.json when available
async function prefill() {
  try {
    const res = await fetch('/config.json', { method: 'GET' });
    if (!res.ok) return;
    const cfg = await res.json();
    if (cfg.ssid) document.getElementById('ssid').value = cfg.ssid;
    if (cfg.devname) document.getElementById('devname').value = cfg.devname;
    if (cfg.ota_password) document.getElementById('ota_password').value = cfg.ota_password;
  if (cfg.reset_hold_seconds) document.getElementById('reset_hold_seconds').value = cfg.reset_hold_seconds;
  } catch (e) {
    // ignore
  }
}

prefill();
// Reset hold seconds UI behavior: show current value and enforce min/max
const resetInput = document.getElementById('reset_hold_seconds');
const resetDisplay = document.getElementById('resetDisplay');
if (resetInput) {
  function updateResetDisplay() {
    let v = parseInt(resetInput.value) || 10;
    if (v < 1) v = 1;
    if (v > 300) v = 300;
    resetInput.value = v;
    if (resetDisplay) resetDisplay.textContent = v;
  }
  resetInput.addEventListener('change', updateResetDisplay);
  resetInput.addEventListener('input', updateResetDisplay);
  // initialize
  updateResetDisplay();
}
