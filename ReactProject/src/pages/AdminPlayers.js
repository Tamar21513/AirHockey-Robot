import React, { useState, useEffect } from "react";
import { Loader2, Search, Mail, Gamepad2, Star, Trophy } from "lucide-react";
import { Input } from "@/components/ui/input";
import { motion } from "framer-motion";
import NeonBadge from "@/components/ui/NeonBadge";
import { esp32Api } from "@/api/esp32";

export default function AdminPlayers() {
  const [users, setUsers] = useState([]);
  const [loading, setLoading] = useState(true);
  const [search, setSearch] = useState("");

  useEffect(() => {
    loadData();
  }, []);

  const loadData = async () => {
    try {
      setLoading(true);
      const allUsers = await esp32Api.getPlayers();
      setUsers(Array.isArray(allUsers) ? allUsers : []);
    } catch (error) {
      console.error("Players load error:", error);
      setUsers([]);
    } finally {
      setLoading(false);
    }
  };

  const filtered = users.filter((u) =>
    (u.full_name || u.name || "").includes(search) || (u.email || "").includes(search)
  );

  if (loading) return <div className="flex items-center justify-center h-64"><Loader2 className="animate-spin text-cyan-400" size={32} /></div>;

  return (
    <div className="max-w-5xl mx-auto space-y-6">
      <div>
        <h1 className="text-2xl font-bold text-white">Manage Players</h1>
        <p className="text-gray-400 mt-1">{users.length} registered users</p>
      </div>

      <div className="relative">
        <Search size={16} className="absolute right-3 top-3 text-gray-500" />
        <Input value={search} onChange={(e) => setSearch(e.target.value)} placeholder="Search by name or email..." className="pr-10 bg-gray-800 border-gray-700 text-white" />
      </div>

      <div className="space-y-3">
        {filtered.map((user, i) => (
          <motion.div
            key={user.id || user.email}
            initial={{ opacity: 0, y: 10 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ delay: i * 0.02 }}
            className="glass-card rounded-xl p-4 border border-white/5 flex flex-col sm:flex-row sm:items-center justify-between gap-3"
          >
            <div className="flex items-center gap-4">
              <div className="w-12 h-12 rounded-full bg-gradient-to-br from-cyan-500 to-purple-600 flex items-center justify-center text-lg font-bold">
                {(user.full_name || user.name || user.email || "?")[0]?.toUpperCase()}
              </div>

              <div>
                <p className="font-medium text-white">{user.full_name || user.name || "—"}</p>
                <p className="text-sm text-gray-400 flex items-center gap-1">
                  <Mail size={12} />
                  {user.email}
                </p>
              </div>
            </div>

            <div className="flex items-center gap-4 text-sm">
              <div className="flex items-center gap-1.5 text-gray-400">
                <Gamepad2 size={14} className="text-cyan-400" />
                <span>{user.games || 0}</span>
              </div>

              <div className="flex items-center gap-1.5 text-gray-400">
                <Star size={14} className="text-yellow-400" />
                <span>{user.highScore || 0}</span>
              </div>

              <div className="flex items-center gap-1.5 text-gray-400">
                <Trophy size={14} className="text-green-400" />
                <span>{user.wins || 0}</span>
              </div>

              <NeonBadge color={user.role === "admin" ? "purple" : "cyan"}>
                {user.role === "admin" ? "Admin" : "Player"}
              </NeonBadge>
            </div>
          </motion.div>
        ))}
      </div>
    </div>
  );
}