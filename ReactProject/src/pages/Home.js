import React, { useState, useEffect } from "react";
import { Link } from "react-router-dom";
import { Gamepad2, Trophy, History, Activity, Gift, ArrowLeft, Zap, Star } from "lucide-react";
import { motion } from "framer-motion";
import StatCard from "@/components/ui/StatCard";
import { useAuth } from "@/lib/AuthContext";
import { esp32Api } from "@/api/esp32";

const quickActions = [
  {
    name: "Book a Game",
    desc: "Choose a date, time and payment option",
    icon: Gamepad2,
    page: "/join",
    color: "from-cyan-600 to-blue-700",
  },
  { name: "Subscriptions", desc: "Join or renew a subscription", icon: Gift, page: "/subscriptions", color: "from-purple-600 to-pink-700" },
  { name: "Leaderboard", desc: "See who is leading", icon: Trophy, page: "/leaderboard", color: "from-yellow-600 to-orange-700" },
  { name: "Live Game", desc: "Watch in real time", icon: Activity, page: "/live", color: "from-green-600 to-emerald-700" },
  { name: "History", desc: "My games", icon: History, page: "/history", color: "from-pink-600 to-rose-700" },
];

export default function Home() {
  const { currentUser } = useAuth();
  const [stats, setStats] = useState({ games: 0, highScore: 0, rank: "-" });

  useEffect(() => {
    loadData();
  }, [currentUser]);

  const loadData = async () => {
    if (!currentUser?.email) return;

    try {
      const data = await esp32Api.getHomeData(currentUser.email);
      setStats({
        games: data?.stats?.games || 0,
        highScore: data?.stats?.highScore || 0,
        rank: data?.stats?.rank || "-",
      });
    } catch (error) {
      console.error("Home load error:", error);
      setStats({ games: 0, highScore: 0, rank: "-" });
    }
  };

  return (
    <div className="max-w-6xl mx-auto space-y-8">
      <motion.div
        initial={{ opacity: 0, y: 20 }}
        animate={{ opacity: 1, y: 0 }}
        className="relative overflow-hidden rounded-3xl bg-gradient-to-br from-gray-900 via-gray-800 to-gray-900 border border-white/10 p-8 md:p-12"
      >
        <div className="relative z-10">
          <div className="flex items-center gap-2 mb-3">
            <Zap size={16} className="text-cyan-400" />
            <span className="text-cyan-400 text-sm font-medium">Welcome</span>
          </div>

          <h1 className="text-3xl md:text-5xl font-bold mb-3">
            <span className="bg-gradient-to-r from-cyan-400 via-blue-400 to-purple-400 bg-clip-text text-transparent">
              {currentUser ? `Hello, ${currentUser.name || "Player"}!` : "AirHockey Pro"}
            </span>
          </h1>
        </div>
      </motion.div>

      {currentUser && (
        <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
          <StatCard title="games" value={stats.games} icon={Gamepad2} color="cyan" />
          <StatCard title="High score" value={stats.highScore} icon={Star} color="purple" />
          <StatCard title="Rank" value={stats.rank} icon={Trophy} color="green" />
        </div>
      )}

      <div>
        <h2 className="text-lg font-semibold text-gray-300 mb-4">Quick actions</h2>

        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4">
          {quickActions.map((action, i) => (
            <motion.div
              key={action.page}
              initial={{ opacity: 0, y: 20 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ delay: 0.1 + i * 0.05 }}
            >
              <Link
                to={action.page}
                className="group block glass-card rounded-2xl p-5 border border-white/5 hover:border-white/15 transition-all duration-300"
              >
                <div className={`w-12 h-12 rounded-xl bg-gradient-to-br ${action.color} flex items-center justify-center mb-4 group-hover:scale-110 transition-transform`}>
                  <action.icon size={22} />
                </div>

                <h3 className="font-semibold text-white mb-1">{action.name}</h3>
                <p className="text-gray-500 text-sm">{action.desc}</p>

                <div className="mt-3 flex items-center text-cyan-400 text-sm opacity-0 group-hover:opacity-100 transition-opacity">
                  <span>Log in</span>
                  <ArrowLeft size={14} className="mr-1" />
                </div>
              </Link>
            </motion.div>
          ))}
        </div>
      </div>
    </div>
  );
}
