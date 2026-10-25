import React, { useState, useEffect } from "react";
import { History, Gamepad2, Trophy, Calendar, Loader2, Star, Target } from "lucide-react";
import { format } from "date-fns";
import { motion } from "framer-motion";
import NeonBadge from "@/components/ui/NeonBadge";
import StatCard from "@/components/ui/StatCard";
import { useAuth } from "@/lib/AuthContext";
import { esp32Api } from "@/api/esp32";

const skillLabels = { beginner: "Beginner", intermediate: "Intermediate", advanced: "Advanced", pro: "Pro" };
const skillColors = { beginner: "green", intermediate: "yellow", advanced: "orange", pro: "red" };
const statusLabels = { scheduled: "Scheduled", in_progress: "In progress", completed: "Completed", cancelled: "Cancelled" };
const statusColors = { scheduled: "cyan", in_progress: "purple", completed: "green", cancelled: "red" };

export default function UserHistory() {
  const { currentUser } = useAuth();
  const [games, setGames] = useState([]);
  const [scores, setScores] = useState([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    loadData();
  }, [currentUser]);

  const loadData = async () => {
    if (!currentUser?.email) return;

    try {
      setLoading(true);
      const data = await esp32Api.getHistory(currentUser.email);
      setGames(Array.isArray(data?.games) ? data.games : []);
      setScores(Array.isArray(data?.scores) ? data.scores : []);
    } catch (error) {
      console.error("History load error:", error);
      setGames([]);
      setScores([]);
    } finally {
      setLoading(false);
    }
  };

  if (loading) {
    return <div className="flex items-center justify-center h-64"><Loader2 className="animate-spin text-cyan-400" size={32} /></div>;
  }

  const highScore = scores.length > 0 ? Math.max(...scores.map((s) => s.score || 0)) : 0;
  const wins = games.filter(
    (g) => g.winner === currentUser?.name || g.winner === currentUser?.email
  ).length;  
  return (
    <div className="max-w-5xl mx-auto space-y-6">
      <div>
        <h1 className="text-2xl font-bold text-white">My Game History</h1>
        <p className="text-gray-400 mt-1">All your games and scores</p>
      </div>

      <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
        <StatCard title="Total games" value={games.length} icon={Gamepad2} color="cyan" />
        <StatCard title="High score" value={highScore} icon={Star} color="purple" />
        <StatCard title="Wins" value={wins} icon={Trophy} color="green" />
      </div>

      <div className="space-y-3">
        <h2 className="text-lg font-semibold text-gray-300">Recent games</h2>

        {games.length === 0 ? (
          <div className="glass-card rounded-2xl p-12 border border-white/5 text-center">
            <History size={48} className="mx-auto mb-3 text-gray-600" />
            <p className="text-gray-500">No games yet</p>
          </div>
        ) : (
          games.map((game, i) => (
            <motion.div
              key={game.id}
              initial={{ opacity: 0, y: 10 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ delay: i * 0.03 }}
              className="glass-card rounded-xl p-4 border border-white/5 flex flex-col sm:flex-row sm:items-center justify-between gap-3"
            >
              <div className="flex items-center gap-4">
                <div className="w-12 h-12 rounded-xl bg-gradient-to-br from-cyan-500/20 to-purple-500/20 flex items-center justify-center">
                  <Target size={20} className="text-cyan-400" />
                </div>

                <div>
                  <div className="flex items-center gap-2">
                    <span className="font-medium text-white">
                      {game.player1_score || 0} - {game.player2_score || 0}
                    </span>
                      {game.winner === currentUser?.name && <NeonBadge color="green">Win!</NeonBadge>}
                  </div>

                  <div className="flex items-center gap-2 text-sm text-gray-400 mt-1">
                    <Calendar size={12} />
                    <span>{game.date ? format(new Date(game.date), "dd/MM/yyyy") : "—"}</span>
                    <span>•</span>
                    <span>{game.time_slot || "—"}</span>
                  </div>
                  <p className="text-xs text-gray-500 mt-1">
                     vs {game.opponent_name || "Robot"}
                  </p>
                </div>
              </div>

              <div className="flex items-center gap-2">
                <NeonBadge color={skillColors[game.skill_level] || "cyan"}>
                  {skillLabels[game.skill_level] || game.skill_level || "—"}
                </NeonBadge>
                <NeonBadge color={statusColors[game.status] || "cyan"}>
                  {statusLabels[game.status] || game.status || "—"}
                </NeonBadge>
              </div>
            </motion.div>
          ))
        )}
      </div>
    </div>
  );
}