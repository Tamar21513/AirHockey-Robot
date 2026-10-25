const ESP32_IP = "192.168.1.103";
const API_BASE = `http://${ESP32_IP}`;

async function parseResponse(res) {
  let data = null;
  let text = "";

  try {
    text = await res.text();
    data = text ? JSON.parse(text) : null;
  } catch {
    data = null;
  }

  if (!res.ok) {
    throw new Error(data?.message || data?.error || text || `HTTP ${res.status}`);
  }

  return data;
}

async function getJson(path) {
  const res = await fetch(`${API_BASE}${path}`);
  return parseResponse(res);
}

async function sendJson(path, method, body) {
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 8000);

  try {
    const res = await fetch(`${API_BASE}${path}`, {
      method,
      headers: { "Content-Type": "application/json" },
      body: body ? JSON.stringify(body) : undefined,
      signal: controller.signal,
    });

    return await parseResponse(res);
  } catch (err) {
    if (err.name === "AbortError") {
      throw new Error("The ESP32 did not respond in time");
    }
    throw err;
  } finally {
    clearTimeout(timeout);
  }
}

export async function sha256Upper(text) {
  const encoder = new TextEncoder();
  const data = encoder.encode(text);
  const hashBuffer = await crypto.subtle.digest("SHA-256", data);
  const hashArray = Array.from(new Uint8Array(hashBuffer));
  return hashArray
    .map((b) => b.toString(16).padStart(2, "0"))
    .join("")
    .toUpperCase();
}

export function normalizeGameCode(code) {
  return String(code || "")
    .trim()
    .replace(/^game_/i, "");
}

export function normalizeGameId(code) {
  const cleanCode = normalizeGameCode(code);
  return cleanCode ? `game_${cleanCode}` : "";
}

export const esp32Api = {
  baseUrl: API_BASE,

  login(payload) {
    return sendJson("/login", "POST", payload);
  },

  register(payload) {
    return sendJson("/register", "POST", payload);
  },

  getHomeData(email) {
    return getJson(`/home-data?email=${encodeURIComponent(email)}`);
  },

  getHistory(email) {
    return getJson(`/history?email=${encodeURIComponent(email)}`);
  },

  getLeaderboard(email) {
    const suffix = email ? `?email=${encodeURIComponent(email)}` : "";
    return getJson(`/leaderboard${suffix}`);
  },

  getAdminDashboard() {
    return getJson("/admin-dashboard");
  },

  getPlayers() {
    return getJson("/players");
  },

  getHighScores() {
    return getJson("/high-scores");
  },

  getBonuses() {
    return getJson("/bonuses");
  },

  getUserBonuses(email) {
    return getJson(`/user-bonuses?email=${encodeURIComponent(email)}`);
  },

  createBonus(payload) {
    return sendJson("/bonuses", "POST", payload);
  },

  deleteBonus(id) {
    return sendJson(`/bonus?id=${encodeURIComponent(id)}`, "DELETE");
  },

  getSubscriptions() {
    return getJson("/subscriptions");
  },

  createSubscription(payload) {
    return sendJson("/subscriptions", "POST", payload);
  },

  updateSubscription(id, payload) {
    return sendJson(`/subscription?id=${encodeURIComponent(id)}`, "PUT", payload);
  },

  deleteSubscription(id) {
    return sendJson(`/subscription?id=${encodeURIComponent(id)}`, "DELETE");
  },

  getUserSubscription(email) {
    return getJson(`/user-subscription?email=${encodeURIComponent(email)}`);
  },

  subscribe(payload) {
    return sendJson("/subscribe", "POST", payload);
  },

  getGamesByDate(date) {
    return getJson(`/games?date=${encodeURIComponent(date)}`);
  },

  updateGame(id, payload) {
    return sendJson(`/game-item?id=${encodeURIComponent(id)}`, "PUT", payload);
  },

  deleteGame(id) {
    return sendJson(`/game-item?id=${encodeURIComponent(id)}`, "DELETE");
  },

  getLiveGame(gameCode = "") {
    const cleanCode = normalizeGameCode(gameCode);
    const suffix = cleanCode ? `?code=${encodeURIComponent(cleanCode)}` : "";
    return getJson(`/game${suffix}`);
  },

  createGame(payload) {
    return sendJson("/game", "POST", payload);
  },

  getGameByCode(code) {
    const cleanCode = normalizeGameCode(code);
    return getJson(`/game-by-code?code=${encodeURIComponent(cleanCode)}`);
  },

  startGame(payload) {
    const cleanCode = normalizeGameCode(
      payload.game_code || payload.code || payload.game_id || payload.id
    );
    const gameId = payload.game_id || payload.id || normalizeGameId(cleanCode);

    return sendJson("/start-game", "POST", {
      ...payload,
      game_code: cleanCode,
      game_id: gameId,
      id: gameId,
    });
  },
};
