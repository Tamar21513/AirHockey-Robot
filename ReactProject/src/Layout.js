import React, { useState } from "react";
import { Link, useNavigate } from "react-router-dom";
import { useAuth } from "./lib/AuthContext";
import {
  Home,
  Gamepad2,
  History,
  Trophy,
  Activity,
  Settings,
  Users,
  Calendar,
  Gift,
  BarChart3,
  Menu,
  X,
  LogOut,
  ChevronDown,
  Shield,
  User as UserIcon,
} from "lucide-react";

const userMenuItems = [
  { name: "Home", icon: Home, page: "/" },
  { name: "Book a Game", icon: Gamepad2, page: "/join" },
  { name: "Subscriptions", icon: Gift, page: "/subscriptions" },
  { name: "History", icon: History, page: "/history" },
  { name: "Leaderboard", icon: Trophy, page: "/leaderboard" },
  { name: "Live Game", icon: Activity, page: "/live" },
];

const adminMenuItems = [
  { name: "Dashboard", icon: BarChart3, page: "/admin" },
  { name: "Manage Subscriptions", icon: Settings, page: "/admin/subscriptions" },
  { name: "Manage Schedule", icon: Calendar, page: "/admin/schedule" },
  { name: "Manage Players", icon: Users, page: "/admin/players" },
  { name: "High Scores & Bonuses", icon: Trophy, page: "/admin/bonuses" },
];

export default function Layout({ children, currentPath = "/" }) {
  const [sidebarOpen, setSidebarOpen] = useState(false);
  const { currentUser, logout } = useAuth();
  const navigate = useNavigate();

  const user = currentUser || {
    name: "Guest User",
    email: "guest@example.com",
    role: "user",
  };

  const isAdmin = user.role === "admin";
  const isAdminPage = currentPath.startsWith("/admin");
  const menuItems = isAdminPage ? adminMenuItems : userMenuItems;

  const handleLogout = () => {
    logout();
    navigate("/login");
  };

  const getUserDisplayName = () => {
    return user.name || user.full_name || user.email || "Guest";
  };

  const getUserInitial = () => {
    const name = getUserDisplayName();
    return name?.[0]?.toUpperCase() || "G";
  };

  return (
    <div dir="ltr" className="min-h-screen bg-gray-950 text-white">
      <nav className="fixed top-0 left-0 right-0 z-50 glass-card border-b border-white/10">
        <div className="max-w-7xl mx-auto px-4 h-16 flex items-center justify-between">
          <div className="flex items-center gap-3">
            <button
              onClick={() => setSidebarOpen(!sidebarOpen)}
              className="lg:hidden p-2 rounded-lg hover:bg-white/10"
            >
              {sidebarOpen ? <X size={20} /> : <Menu size={20} />}
            </button>

            <Link to="/" className="flex items-center gap-2">
              <Gamepad2 size={20} />
              <span className="text-xl font-bold text-cyan-400">AirHockey Robot</span>
            </Link>
          </div>

          <div className="flex items-center gap-3">
            {isAdmin && (
              <Link to={isAdminPage ? "/" : "/admin"}>
                <button className="px-3 py-2 border border-purple-500/50 rounded-lg text-purple-400 hover:bg-purple-500/10 flex items-center gap-1">
                  {isAdminPage ? (
                    <>
                      <UserIcon size={14} />
                      User Mode
                    </>
                  ) : (
                    <>
                      <Shield size={14} />
                      Admin Mode
                    </>
                  )}
                </button>
              </Link>
            )}

            <div className="relative group">
              <button className="flex items-center gap-2 px-3 py-2 rounded-lg hover:bg-white/10">
                <div className="w-8 h-8 rounded-full bg-purple-600 flex items-center justify-center">
                  {getUserInitial()}
                </div>

                <span className="text-sm hidden sm:inline">{getUserDisplayName()}</span>
                <ChevronDown size={14} />
              </button>

              <div className="absolute left-0 mt-2 w-56 bg-gray-900 border border-gray-700 rounded-xl shadow-lg opacity-0 invisible group-hover:opacity-100 group-hover:visible transition-all">
                <div className="px-4 py-3 text-gray-400 text-xs border-b border-gray-700">
                  {user.email}
                </div>

                <button
                  onClick={handleLogout}
                  className="w-full text-right px-4 py-3 text-red-400 hover:bg-red-500/10 flex items-center gap-2"
                >
                  <LogOut size={14} />
                  Log out
                </button>
              </div>
            </div>
          </div>
        </div>
      </nav>

      <aside
        className={`fixed top-16 right-0 bottom-0 w-64 glass-card border-l border-white/10 z-40 transition-transform duration-300 lg:translate-x-0 ${
          sidebarOpen ? "translate-x-0" : "translate-x-full"
        }`}
      >
        <div className="p-4 space-y-1">
          {menuItems.map((item) => {
            const Icon = item.icon;

            return (
              <Link
                key={item.page}
                to={item.page}
                onClick={() => setSidebarOpen(false)}
                className="flex items-center gap-3 px-4 py-3 rounded-xl text-gray-400 hover:text-white hover:bg-white/5"
              >
                <Icon size={18} />
                <span className="text-sm font-medium">{item.name}</span>
              </Link>
            );
          })}
        </div>
      </aside>

      {sidebarOpen && (
        <div
          className="fixed inset-0 bg-black/50 z-30 lg:hidden"
          onClick={() => setSidebarOpen(false)}
        />
      )}

      <main className="pt-16 lg:pr-64 min-h-screen">
        <div className="p-6">{children}</div>
      </main>
    </div>
  );
}