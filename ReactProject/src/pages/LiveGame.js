import React, { useState, useEffect } from "react";
import { Activity, Loader2, Zap, Timer, Gauge, Trophy, Gamepad2 } from "lucide-react";
import { motion, AnimatePresence } from "framer-motion";
import Confetti from "react-confetti";
import { useWindowSize } from "react-use";
import NeonBadge from "@/components/ui/NeonBadge";
import { esp32Api } from "@/api/esp32";
import { useAuth } from "@/lib/AuthContext";

function formatTime(seconds) {
  const sec = Number(seconds || 0);
  return `${Math.floor(sec / 60)}:${String(sec % 60).padStart(2, "0")}`;
}

function normalizeGame(data) {
  if (!data) return null;

  const game = data.game || data;

  return {
    id: game.id || game.game_id || 1,
    game_code: game.game_code || "",
    table_number: game.table_number || 1,
    player1_name: game.player1_name || game.player_name || "Player",
    player1_score: game.player1_score || 0,
    player2_score: game.player2_score || 0,
    puck_speed: game.puck_speed || 0,
    game_time_seconds: game.game_time_seconds || 0,
    last_goal_by: game.last_goal_by || "none",
    status: game.status || "idle",
    winner: game.winner || "",
    is_active: game.is_active ?? game.status === "in_progress",
    skill_level: game.skill_level || 1,
    game_mode: game.game_mode || "first_to_7",
  };
}

export default function LiveGame() {
  const { activeGame } = useAuth();
  const [liveGames, setLiveGames] = useState([]);
  const [loading, setLoading] = useState(true);
  const { width, height } = useWindowSize();

  const loadGameData = async () => {
    try {
      const data = await esp32Api.getLiveGame(activeGame?.game_code || "");
      const game = normalizeGame(data);

      if (!game) {
        setLiveGames([]);
      } else if (game.is_active === false && game.status !== "completed") {
        setLiveGames([]);
      } else {
        setLiveGames([game]);
      }
    } catch (err) {
      console.error("ESP32 connection error:", err);
      if (activeGame) {
        setLiveGames([normalizeGame(activeGame)]);
      } else {
        setLiveGames([]);
      }
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadGameData();
    const interval = setInterval(loadGameData, 1000);
    return () => clearInterval(interval);
  }, [activeGame?.game_code]);

  const hasWinner = liveGames.some((g) => g.status === "completed" && g.winner);

  if (loading) {
    return (
      <div className="flex items-center justify-center h-64">
        <Loader2 className="animate-spin text-cyan-400" size={32} />
      </div>
    );
  }

  return (
    <div className="max-w-5xl mx-auto space-y-6">
      {hasWinner && <Confetti width={width} height={height} />}

      <div className="flex items-center gap-3">
        <h1 className="text-2xl font-bold text-white">Live Game</h1>

        {liveGames.some((g) => g.status === "in_progress") && (
          <div className="flex items-center gap-1.5">
            <span className="w-2.5 h-2.5 rounded-full bg-red-500 animate-pulse" />
            <span className="text-red-400 text-sm font-medium">LIVE</span>
          </div>
        )}
      </div>

      {liveGames.length === 0 ? (
        <div className="glass-card rounded-2xl p-16 border border-white/5 text-center">
          <Activity size={56} className="mx-auto mb-4 text-gray-600" />
          <p className="text-gray-400 text-lg">No active game right now</p>
          <p className="text-gray-600 text-sm mt-1">After a game is started with a code, it will appear here in real time</p>
        </div>
      ) : (
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          <AnimatePresence>
            {liveGames.map((game) => (
              <motion.div
                key={game.id}
                initial={{ opacity: 0, scale: 0.95 }}
                animate={{ opacity: 1, scale: 1 }}
                exit={{ opacity: 0, scale: 0.95 }}
                className={`glass-card rounded-2xl border overflow-hidden ${
                  game.status === "completed" ? "border-yellow-500/30" : "border-cyan-500/20"
                }`}
              >
                <div
                  className={`p-4 border-b border-white/5 flex items-center justify-between ${
                    game.status === "completed"
                      ? "bg-gradient-to-l from-yellow-500/10 to-transparent"
                      : "bg-gradient-to-l from-cyan-500/5 to-transparent"
                  }`}
                >
                  <div className="flex items-center gap-2">
                    <div
                      className={`w-3 h-3 rounded-full ${
                        game.status === "completed" ? "bg-yellow-400" : "bg-green-500 animate-pulse"
                      }`}
                    />
                    <span className="text-sm font-medium text-gray-300">
                      Table #{game.table_number}
                      {game.game_code ? ` • Code ${game.game_code}` : ""}
                    </span>
                  </div>

                  <NeonBadge color={game.status === "completed" ? "yellow" : "cyan"}>
                    {game.status === "completed" ? "Completed" : "Active"}
                  </NeonBadge>
                </div>

                <div className="p-6">
                  <div className="flex items-center justify-between">
                    <div className="text-center flex-1">
                      <p className="text-gray-400 text-sm mb-2">{game.player1_name || "Player"}</p>
                      <motion.p
                        key={game.player1_score}
                        initial={{ scale: 1.3 }}
                        animate={{ scale: 1 }}
                        className="text-5xl font-black text-cyan-400"
                      >
                        {game.player1_score}
                      </motion.p>
                    </div>

                    <div className="px-6">
                      <div className="w-16 h-16 rounded-full border-2 border-gray-700 flex items-center justify-center">
                        <span className="text-gray-500 text-lg font-bold">VS</span>
                      </div>
                    </div>

                    <div className="text-center flex-1">
                      <p className="text-gray-400 text-sm mb-2">Robot</p>
                      <motion.p
                        key={game.player2_score}
                        initial={{ scale: 1.3 }}
                        animate={{ scale: 1 }}
                        className="text-5xl font-black text-purple-400"
                      >
                        {game.player2_score}
                      </motion.p>
                    </div>
                  </div>

                  <p className="text-xs text-gray-500 mt-4 text-center">
                    {game.game_mode === "timer_3_minutes" ? "The game is limited to 3 minutes" : "The game ends at 7 points"}
                  </p>

                  {game.status === "completed" && game.winner && (
                    <motion.div
                      initial={{ opacity: 0, y: 10 }}
                      animate={{ opacity: 1, y: 0 }}
                      className="mt-6 rounded-2xl border border-yellow-500/30 bg-yellow-500/10 p-6 text-center"
                    >
                      <Trophy size={34} className="mx-auto mb-3 text-yellow-400" />
                      <p className="text-sm text-yellow-300 mb-2">Game over</p>
                      <p className="text-3xl font-black text-yellow-400">🏆 Winner: {game.winner}</p>
                    </motion.div>
                  )}
                </div>

                <div className="grid grid-cols-4 border-t border-white/5">
                  <div className="p-3 text-center border-l border-white/5">
                    <Timer size={14} className="mx-auto text-gray-500 mb-1" />
                    <p className="text-sm font-medium text-white">{formatTime(game.game_time_seconds)}</p>
                    <p className="text-xs text-gray-500">Time</p>
                  </div>

                  <div className="p-3 text-center border-l border-white/5">
                    <Gauge size={14} className="mx-auto text-gray-500 mb-1" />
                    <p className="text-sm font-medium text-white">{game.puck_speed || 0}</p>
                    <p className="text-xs text-gray-500">Speed</p>
                  </div>

                  <div className="p-3 text-center border-l border-white/5">
                    <Gamepad2 size={14} className="mx-auto text-gray-500 mb-1" />
                    <p className="text-sm font-medium text-white">{game.skill_level}</p>
                    <p className="text-xs text-gray-500">Level</p>
                  </div>

                  <div className="p-3 text-center">
                    <Zap size={14} className="mx-auto text-gray-500 mb-1" />
                    <p className="text-sm font-medium text-white">
                      {game.last_goal_by === "player1" ? "Player" : game.last_goal_by === "player2" ? "Robot" : "—"}
                    </p>
                    <p className="text-xs text-gray-500">Last goal</p>
                  </div>
                </div>
              </motion.div>
            ))}
          </AnimatePresence>
        </div>
      )}
    </div>
  );
}
