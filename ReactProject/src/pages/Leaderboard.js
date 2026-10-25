import React, { useState, useEffect } from "react";
import { Trophy, Medal, Star, Loader2, User, Crown, TrendingUp } from "lucide-react";
import { motion } from "framer-motion";
import { useAuth } from "@/lib/AuthContext";
import { esp32Api } from "@/api/esp32";

export default function Leaderboard() {
  const { currentUser } = useAuth();
  const [leaderboard, setLeaderboard] = useState([]);
  const [loading, setLoading] = useState(true);
  const [myRank, setMyRank] = useState(null);

  useEffect(() => {
    loadData();
  }, [currentUser]);

  const loadData = async () => {
    try {
      setLoading(true);
      const data = await esp32Api.getLeaderboard(currentUser?.email);
      setLeaderboard(Array.isArray(data?.leaderboard) ? data.leaderboard : []);
      setMyRank(data?.myRank || null);
    } catch (error) {
      console.error("Leaderboard load error:", error);
      setLeaderboard([]);
      setMyRank(null);
    } finally {
      setLoading(false);
    }
  };

  const podiumColors = [
    "from-yellow-500/30 to-yellow-500/5 border-yellow-500/50",
    "from-gray-400/30 to-gray-400/5 border-gray-400/50",
    "from-orange-600/30 to-orange-600/5 border-orange-600/50",
  ];
  const podiumIcons = [Crown, Medal, Medal];

  if (loading) return <div className="flex items-center justify-center h-64"><Loader2 className="animate-spin text-cyan-400" size={32} /></div>;

  return (
    <div className="max-w-4xl mx-auto space-y-6">
      <div className="flex items-center justify-between flex-wrap gap-4">
        <div>
          <h1 className="text-2xl font-bold text-white">Leaderboard</h1>
          <p className="text-gray-400 mt-1">The best players against the robot</p>
        </div>

        {myRank && (
          <div className="glass-card rounded-xl px-4 py-2 border border-cyan-500/30 flex items-center gap-2">
            <TrendingUp size={16} className="text-cyan-400" />
            <span className="text-sm text-gray-300">Your rank:</span>
            <span className="text-cyan-400 font-bold">#{myRank}</span>
          </div>
        )}
      </div>

      {leaderboard.length >= 3 && (
        <div className="grid grid-cols-3 gap-3">
          {[1, 0, 2].map((idx) => {
            const player = leaderboard[idx];
            const Icon = podiumIcons[idx];
            if (!player) return null;

            return (
              <motion.div
                key={player.user_email || idx}
                initial={{ opacity: 0, y: 20 }}
                animate={{ opacity: 1, y: 0 }}
                transition={{ delay: idx * 0.1 }}
                className={`glass-card rounded-2xl p-5 border text-center bg-gradient-to-br ${podiumColors[idx]} ${idx === 0 ? "scale-105 sm:scale-110" : ""}`}
              >
                <div className="w-14 h-14 rounded-full bg-gray-800 mx-auto flex items-center justify-center mb-3 border-2 border-current">
                  <Icon size={idx === 0 ? 24 : 18} className={idx === 0 ? "text-yellow-400" : idx === 1 ? "text-gray-300" : "text-orange-400"} />
                </div>
                <p className="font-bold text-white text-sm truncate">{player.user_name || player.user_email?.split("@")[0]}</p>
                <p className="text-2xl font-black mt-1 bg-gradient-to-r from-cyan-400 to-purple-400 bg-clip-text text-transparent">{player.score}</p>
                <p className="text-xs text-gray-500 mt-1">Place #{idx + 1}</p>
              </motion.div>
            );
          })}
        </div>
      )}

      <div className="glass-card rounded-2xl border border-white/5 overflow-hidden">
        <div className="p-4 border-b border-white/5">
          <h2 className="font-semibold text-gray-300">All players</h2>
        </div>

        <div className="divide-y divide-white/5">
          {leaderboard.map((player, i) => (
            <motion.div
              key={player.user_email || i}
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              transition={{ delay: i * 0.02 }}
              className={`flex items-center justify-between p-4 hover:bg-white/5 transition-colors ${player.user_email === currentUser?.email ? "bg-cyan-500/5" : ""}`}
            >
              <div className="flex items-center gap-4">
                <span className={`w-8 h-8 rounded-lg flex items-center justify-center text-sm font-bold ${i < 3 ? "bg-gradient-to-br from-cyan-500 to-purple-600 text-white" : "bg-gray-800 text-gray-400"}`}>
                  {i + 1}
                </span>

                <div className="w-9 h-9 rounded-full bg-gray-800 flex items-center justify-center">
                  <User size={16} className="text-gray-400" />
                </div>

                <div>
                  <p className="font-medium text-white text-sm">
                    {player.user_name || player.user_email?.split("@")[0]}
                    {player.user_email === currentUser?.email && <span className="text-cyan-400 text-xs mr-2">(you)</span>}
                  </p>
                  {player.skill_level && <p className="text-xs text-gray-500">{player.skill_level}</p>}
                </div>
              </div>

              <div className="flex items-center gap-3">
                {player.is_high_score && <Star size={14} className="text-yellow-400" />}
                <span className="text-lg font-bold text-white">{player.score}</span>
              </div>
            </motion.div>
          ))}

          {leaderboard.length === 0 && (
            <div className="p-12 text-center">
              <Trophy size={48} className="mx-auto mb-3 text-gray-600" />
              <p className="text-gray-500">No high scores yet</p>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}