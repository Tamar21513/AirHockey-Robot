import React from "react";
import { Link } from "react-router-dom";
import { Home, Gamepad2 } from "lucide-react";
import { Button } from "@/components/ui/button";

export default function PageNotFound() {
  return (
    <div dir="ltr" className="min-h-screen bg-gray-950 flex items-center justify-center p-6">
      <div className="text-center">

        <div className="w-20 h-20 rounded-2xl bg-gradient-to-br from-cyan-500/20 to-purple-500/20 border border-cyan-500/30 flex items-center justify-center mx-auto mb-6">
          <Gamepad2 size={36} className="text-cyan-400" />
        </div>

        <h1 className="text-6xl font-black text-white mb-2">
          404
        </h1>

        <p className="text-gray-400 text-lg mb-8">
          The page you were looking for was not found
        </p>

        <Link to="/">
          <Button className="bg-gradient-to-r from-cyan-600 to-purple-600 hover:opacity-90">
            <Home size={16} className="ml-2" />
            Back to home
          </Button>
        </Link>

      </div>
    </div>
  );
}