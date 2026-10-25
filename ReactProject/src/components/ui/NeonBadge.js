import React from "react";

const colorMap = {
  cyan: "bg-cyan-500/10 text-cyan-400 border-cyan-500/30",
  purple: "bg-purple-500/10 text-purple-400 border-purple-500/30",
  green: "bg-green-500/10 text-green-400 border-green-500/30",
  pink: "bg-pink-500/10 text-pink-400 border-pink-500/30",
  orange: "bg-orange-500/10 text-orange-400 border-orange-500/30",
  red: "bg-red-500/10 text-red-400 border-red-500/30",
  yellow: "bg-yellow-500/10 text-yellow-400 border-yellow-500/30",
};

export default function NeonBadge({ children, color = "cyan", className = "" }) {
  return (
    <span className={`inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium border ${colorMap[color]} ${className}`}>
      {children}
    </span>
  );
}