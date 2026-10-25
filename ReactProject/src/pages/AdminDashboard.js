import React, { useState, useEffect } from "react";
import { Users, Gamepad2, DollarSign, Loader2, TrendingUp, Clock } from "lucide-react";
import StatCard from "@/components/ui/StatCard";
import { BarChart, Bar, XAxis, YAxis, Tooltip, ResponsiveContainer, PieChart, Pie, Cell } from "recharts";
import { motion } from "framer-motion";
import { format, subDays } from "date-fns";
import { esp32Api } from "@/api/esp32";

const CHART_COLORS = ["#00d4ff", "#a855f7", "#22c55e", "#ec4899", "#f59e0b"];

export default function AdminDashboard() {
  const [stats, setStats] = useState({
    games: 0,
    users: 0,
    revenue: 0,
    activeSubs: 0,
  });

  const [games, setGames] = useState([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    loadData();
  }, []);

  const loadData = async () => {
    try {
      setLoading(true);
      const data = await esp32Api.getAdminDashboard();

      setGames(Array.isArray(data?.games) ? data.games : []);
      setStats({
        games: data?.stats?.games || 0,
        users: data?.stats?.users || 0,
        revenue: data?.stats?.revenue || 0,
        activeSubs: data?.stats?.activeSubs || 0,
      });
    } catch (err) {
      console.error("Dashboard load error:", err);
      setGames([]);
      setStats({ games: 0, users: 0, revenue: 0, activeSubs: 0 });
    } finally {
      setLoading(false);
    }
  };

  const last7Days = Array.from({ length: 7 }, (_, i) => {
    const d = subDays(new Date(), 6 - i);
    const dayStr = format(d, "yyyy-MM-dd");

    return {
      name: format(d, "dd/MM"),
      count: games.filter((g) => g.date === dayStr).length,
    };
  });

  const skillDistribution = [
    { key: "beginner", label: "Beginner" },
    { key: "intermediate", label: "Intermediate" },
    { key: "advanced", label: "Advanced" },
    { key: "pro", label: "Pro" },
  ]
    .map((level) => ({
      name: level.label,
      value: games.filter((g) => g.skill_level === level.key).length,
    }))
    .filter((d) => d.value > 0);

  const hourDistribution = {};

  games.forEach((g) => {
    if (g.time_slot) {
      const hour = String(g.time_slot).split(":")[0];
      hourDistribution[hour] = (hourDistribution[hour] || 0) + 1;
    }
  });

  const peakData = Object.entries(hourDistribution)
    .sort(([a], [b]) => a.localeCompare(b))
    .map(([hour, count]) => ({
      name: `${hour}:00`,
      count,
    }));

  if (loading) {
    return (
      <div className="flex items-center justify-center h-64">
        <Loader2 className="animate-spin text-cyan-400" size={32} />
      </div>
    );
  }

  return (
    <div className="max-w-6xl mx-auto space-y-6">
      <h1 className="text-2xl font-bold text-white">Dashboard</h1>

      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
        <StatCard title="Total games" value={stats.games} icon={Gamepad2} color="cyan" />
        <StatCard title="Users" value={stats.users} icon={Users} color="purple" />
        <StatCard title="Revenue" value={`₪${Number(stats.revenue).toLocaleString()}`} icon={DollarSign} color="green" />
        <StatCard title="Active subscriptions" value={stats.activeSubs} icon={TrendingUp} color="pink" />
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} className="glass-card rounded-2xl p-6 border border-white/5">
          <h3 className="font-semibold text-gray-300 mb-4">Games - last 7 days</h3>
          <ResponsiveContainer width="100%" height={200}>
            <BarChart data={last7Days}>
              <XAxis dataKey="name" tick={{ fill: "#9ca3af", fontSize: 12 }} axisLine={false} tickLine={false} />
              <YAxis tick={{ fill: "#9ca3af", fontSize: 12 }} axisLine={false} tickLine={false} />
              <Tooltip />
              <Bar dataKey="count" fill="#00d4ff" radius={[6, 6, 0, 0]} />
            </BarChart>
          </ResponsiveContainer>
        </motion.div>

        <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ delay: 0.1 }} className="glass-card rounded-2xl p-6 border border-white/5">
          <h3 className="font-semibold text-gray-300 mb-4">Breakdown by level</h3>
          <ResponsiveContainer width="100%" height={200}>
            <PieChart>
              <Pie data={skillDistribution} outerRadius={80} dataKey="value">
                {skillDistribution.map((_, i) => (
                  <Cell key={i} fill={CHART_COLORS[i % CHART_COLORS.length]} />
                ))}
              </Pie>
              <Tooltip />
            </PieChart>
          </ResponsiveContainer>
        </motion.div>

        <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ delay: 0.2 }} className="glass-card rounded-2xl p-6 border border-white/5 lg:col-span-2">
          <div className="flex items-center gap-2 mb-4">
            <Clock size={18} className="text-orange-400" />
            <h3 className="font-semibold text-gray-300">Popular hours</h3>
          </div>

          <ResponsiveContainer width="100%" height={200}>
            <BarChart data={peakData}>
              <XAxis dataKey="name" tick={{ fill: "#9ca3af", fontSize: 12 }} axisLine={false} tickLine={false} />
              <YAxis tick={{ fill: "#9ca3af", fontSize: 12 }} axisLine={false} tickLine={false} />
              <Tooltip />
              <Bar dataKey="count" fill="#a855f7" radius={[6, 6, 0, 0]} />
            </BarChart>
          </ResponsiveContainer>
        </motion.div>
      </div>
    </div>
  );
}