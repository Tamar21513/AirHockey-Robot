import React from "react";

export default function StatCard({ title, value, icon: Icon, color, trend }) {

  const colorMap = {
    cyan: "from-cyan-500/20 to-cyan-500/5 border-cyan-500/30 text-cyan-400",
    purple: "from-purple-500/20 to-purple-500/5 border-purple-500/30 text-purple-400",
    green: "from-green-500/20 to-green-500/5 border-green-500/30 text-green-400",
    pink: "from-pink-500/20 to-pink-500/5 border-pink-500/30 text-pink-400",
    orange: "from-orange-500/20 to-orange-500/5 border-orange-500/30 text-orange-400",
  };

  const styles = colorMap[color] || colorMap.cyan;

  return (
    <div className={`glass-card rounded-2xl p-5 border bg-gradient-to-br ${styles}`}>
      <div className="flex items-start justify-between">

        <div>
          <p className="text-gray-400 text-sm mb-1">{title}</p>
          <p className="text-2xl font-bold text-white">{value}</p>

          {trend && (
            <p className="text-xs mt-1 text-green-400">
              {trend}
            </p>
          )}
        </div>

        {Icon && (
          <div className={`p-3 rounded-xl bg-gradient-to-br ${styles.split(' ').slice(0,2).join(' ')}`}>
            <Icon size={20} />
          </div>
        )}

      </div>
    </div>
  );
}