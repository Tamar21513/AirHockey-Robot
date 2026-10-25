export const createPageUrl = (pageName, params = "") => {
  const routes = {
    Home: "/",
    JoinGame: "/join",
    Subscriptions: "/subscriptions",
    UserHistory: "/history",
    Leaderboard: "/leaderboard",
    LiveGame: "/live",
    AdminDashboard: "/admin",
    AdminSubscriptions: "/admin/subscriptions",
    AdminSchedule: "/admin/schedule",
    AdminPlayers: "/admin/players",
    AdminBonuses: "/admin/bonuses",
  };
  return (routes[pageName] || `/${pageName}`) + (params ? `?${params}` : "");
};