import React, { useState, useEffect } from "react";
import { Trophy, Gift, Loader2, Star, Plus, Send, Trash2 } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { Textarea } from "@/components/ui/textarea";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";
import { Dialog, DialogContent, DialogHeader, DialogTitle, DialogFooter } from "@/components/ui/dialog";
import { toast } from "sonner";
import { motion } from "framer-motion";
import NeonBadge from "@/components/ui/NeonBadge";
import { esp32Api } from "@/api/esp32";

const bonusTypeLabels = { high_score: "New high score", streak: "Streak", tournament: "Tournament", loyalty: "Loyalty" };
const bonusTypeColors = { high_score: "yellow", streak: "purple", tournament: "cyan", loyalty: "green" };

export default function AdminBonuses() {
  const [scores, setScores] = useState([]);
  const [bonuses, setBonuses] = useState([]);
  const [loading, setLoading] = useState(true);
  const [dialogOpen, setDialogOpen] = useState(false);
  const [form, setForm] = useState({ user_email: "", user_name: "", type: "high_score", amount: "", description: "", game_id: "" });
  const [saving, setSaving] = useState(false);

  useEffect(() => {
    loadData();
  }, []);

  const loadData = async () => {
    try {
      setLoading(true);
      const [allScores, allBonuses] = await Promise.all([
        esp32Api.getHighScores(),
        esp32Api.getBonuses(),
      ]);
      setScores(Array.isArray(allScores) ? allScores : []);
      setBonuses(Array.isArray(allBonuses) ? allBonuses : []);
    } catch (error) {
      console.error("Bonuses load error:", error);
      toast.error("Failed to load bonuses");
      setScores([]);
      setBonuses([]);
    } finally {
      setLoading(false);
    }
  };

  const handleAwardBonus = async () => {
    try {
      setSaving(true);
      await esp32Api.createBonus({
        ...form,
        amount: Number(form.amount),
      });
      toast.success("Bonus granted successfully!");
      setDialogOpen(false);
      setForm({ user_email: "", user_name: "", type: "high_score", amount: "", description: "", game_id: "" });
      loadData();
    } catch (error) {
      console.error("Bonus create error:", error);
      toast.error("Failed to send the bonus");
    } finally {
      setSaving(false);
    }
  };

  const awardBonusForScore = (score) => {
    setForm({
      user_email: score.user_email,
      user_name: score.user_name || score.user_email?.split("@")[0],
      type: "high_score",
      amount: "",
      description: `Bonus for a new high score: ${score.score} points`,
      game_id: score.game_id || "",
    });
    setDialogOpen(true);
  };

  const handleDeleteBonus = async (id) => {
    try {
      await esp32Api.deleteBonus(id);
      toast.success("Bonus deleted");
      loadData();
    } catch (error) {
      console.error("Bonus delete error:", error);
      toast.error("Failed to delete the bonus");
    }
  };

  if (loading) return <div className="flex items-center justify-center h-64"><Loader2 className="animate-spin text-cyan-400" size={32} /></div>;

  return (
    <div className="max-w-5xl mx-auto space-y-6">
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-2xl font-bold text-white">High Scores & Bonuses</h1>
          <p className="text-gray-400 mt-1">Review high scores and grant bonuses</p>
        </div>

        <Button onClick={() => setDialogOpen(true)} className="bg-gradient-to-r from-cyan-600 to-purple-600 hover:opacity-90">
          <Plus size={16} className="ml-2" />
          Give bonus
        </Button>
      </div>

      <div>
        <h2 className="text-lg font-semibold text-gray-300 mb-3 flex items-center gap-2">
          <Star size={18} className="text-yellow-400" />
          Recent high scores
        </h2>

        {scores.length === 0 ? (
          <div className="glass-card rounded-2xl p-8 border border-white/5 text-center">
            <Trophy size={40} className="mx-auto mb-2 text-gray-600" />
            <p className="text-gray-500">No high scores reported</p>
          </div>
        ) : (
          <div className="space-y-3">
            {scores.map((score, i) => (
              <motion.div
                key={score.id}
                initial={{ opacity: 0, y: 10 }}
                animate={{ opacity: 1, y: 0 }}
                transition={{ delay: i * 0.03 }}
                className="glass-card rounded-xl p-4 border border-yellow-500/20 flex flex-col sm:flex-row sm:items-center justify-between gap-3"
              >
                <div className="flex items-center gap-4">
                  <div className="w-12 h-12 rounded-xl bg-gradient-to-br from-yellow-500/20 to-orange-500/20 flex items-center justify-center">
                    <Trophy size={20} className="text-yellow-400" />
                  </div>

                  <div>
                    <p className="font-medium text-white">{score.user_name || score.user_email?.split("@")[0]}</p>
                    <p className="text-sm text-gray-400">{score.user_email}</p>
                  </div>
                </div>

                <div className="flex items-center gap-3">
                  <span className="text-2xl font-black text-yellow-400">{score.score}</span>
                  <Button variant="outline" size="sm" onClick={() => awardBonusForScore(score)} className="border-green-500/30 text-green-400 hover:bg-green-500/10 text-xs">
                    <Gift size={12} className="ml-1" />
                    Give bonus
                  </Button>
                </div>
              </motion.div>
            ))}
          </div>
        )}
      </div>

      <div>
        <h2 className="text-lg font-semibold text-gray-300 mb-3 flex items-center gap-2">
          <Gift size={18} className="text-green-400" />
          Granted bonuses
        </h2>

        {bonuses.length === 0 ? (
          <div className="glass-card rounded-2xl p-8 border border-white/5 text-center">
            <Gift size={40} className="mx-auto mb-2 text-gray-600" />
            <p className="text-gray-500">No bonuses</p>
          </div>
        ) : (
          <div className="space-y-3">
            {bonuses.map((bonus, i) => (
              <motion.div
                key={bonus.id}
                initial={{ opacity: 0, y: 10 }}
                animate={{ opacity: 1, y: 0 }}
                transition={{ delay: i * 0.03 }}
                className="glass-card rounded-xl p-4 border border-white/5 flex flex-col sm:flex-row sm:items-center justify-between gap-3"
              >
                <div className="flex items-center gap-4">
                  <div className="w-10 h-10 rounded-xl bg-gradient-to-br from-green-500/20 to-emerald-500/20 flex items-center justify-center">
                    <Gift size={16} className="text-green-400" />
                  </div>

                  <div>
                    <div className="flex items-center gap-2">
                      <span className="font-medium text-white">{bonus.user_name || bonus.user_email?.split("@")[0]}</span>
                      <NeonBadge color={bonusTypeColors[bonus.type] || "green"}>
                        {bonusTypeLabels[bonus.type] || bonus.type}
                      </NeonBadge>
                    </div>
                    <p className="text-sm text-gray-400">{bonus.description}</p>
                  </div>
                </div>

                <div className="flex items-center gap-3">
                  <span className="text-lg font-bold text-green-400">₪{bonus.amount}</span>
                  <NeonBadge color={bonus.is_claimed ? "green" : "orange"}>
                    {bonus.is_claimed ? "Redeemed" : "Active"}
                  </NeonBadge>
                  <Button variant="ghost" size="icon" onClick={() => handleDeleteBonus(bonus.id)} className="text-gray-400 hover:text-red-400">
                    <Trash2 size={14} />
                  </Button>
                </div>
              </motion.div>
            ))}
          </div>
        )}
      </div>

      <Dialog open={dialogOpen} onOpenChange={setDialogOpen}>
        <DialogContent className="bg-gray-900 border-gray-700 text-white max-w-md">
          <DialogHeader>
            <DialogTitle>Grant Bonus</DialogTitle>
          </DialogHeader>

          <div className="space-y-4">
            <div>
              <Label>User email</Label>
              <Input value={form.user_email} onChange={(e) => setForm({ ...form, user_email: e.target.value })} className="bg-gray-800 border-gray-700 text-white mt-1" />
            </div>

            <div>
              <Label>User name</Label>
              <Input value={form.user_name} onChange={(e) => setForm({ ...form, user_name: e.target.value })} className="bg-gray-800 border-gray-700 text-white mt-1" />
            </div>

            <div>
              <Label>Bonus type</Label>
              <Select value={form.type} onValueChange={(v) => setForm({ ...form, type: v })}>
                <SelectTrigger className="bg-gray-800 border-gray-700 text-white mt-1"><SelectValue /></SelectTrigger>
                <SelectContent className="bg-gray-900 border-gray-700">
                  {Object.entries(bonusTypeLabels).map(([k, v]) => (
                    <SelectItem key={k} value={k} className="text-white focus:bg-gray-800">
                      {v}
                    </SelectItem>
                  ))}
                </SelectContent>
              </Select>
            </div>

            <div>
              <Label>Amount (₪)</Label>
              <Input type="number" value={form.amount} onChange={(e) => setForm({ ...form, amount: e.target.value })} className="bg-gray-800 border-gray-700 text-white mt-1" />
            </div>

            <div>
              <Label>Description</Label>
              <Textarea value={form.description} onChange={(e) => setForm({ ...form, description: e.target.value })} className="bg-gray-800 border-gray-700 text-white mt-1" rows={2} />
            </div>
          </div>

          <DialogFooter>
            <Button variant="outline" onClick={() => setDialogOpen(false)} className="border-gray-700 text-gray-300">Cancel</Button>
            <Button onClick={handleAwardBonus} disabled={!form.user_email || !form.amount || saving} className="bg-gradient-to-r from-cyan-600 to-purple-600">
              {saving && <Loader2 className="animate-spin ml-2" size={16} />}
              <Send size={14} className="ml-2" />
              Send bonus
            </Button>
          </DialogFooter>
        </DialogContent>
      </Dialog>
    </div>
  );
}