// Thin client for the routes implemented in solverApi.js.
async function request(path, options = {}) {
  const res = await fetch(path, {
    headers: { 'Content-Type': 'application/json' },
    ...options,
  });
  let data = null;
  try {
    data = await res.json();
  } catch {
    /* non-JSON error body */
  }
  if (!res.ok) throw new Error((data && data.error) || `Request failed (${res.status})`);
  return data;
}

export const api = {
  health: () => request('/api/health'),
  config: () => request('/api/config'),
  generate: (difficulty, signal) =>
    request('/api/generate', { method: 'POST', body: JSON.stringify({ difficulty }), signal }),
  analyze: (puzzle, signal) =>
    request('/api/analyze', { method: 'POST', body: JSON.stringify({ puzzle }), signal }),
};
