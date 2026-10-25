import React, { useState, useEffect } from "react";
import { Calendar as CalendarIcon, Loader2, Pencil, Trash2, Clock, User } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Calendar } from "@/components/ui/calendar";
import { Popover, PopoverContent, PopoverTrigger } from "@/components/ui/popover";
import { Dialog, DialogContent, DialogHeader, DialogTitle, DialogFooter } from "@/components/ui/dialog";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";
import { format } from "date-fns";
import { toast } from "sonner";
import { motion } from "framer-motion";
import NeonBadge from "@/components/ui/NeonBadge";
import { esp32Api } from "@/api/esp32";

const statusLabels = { scheduled: "Scheduled", in_progress: "In progress", completed: "Completed", cancelled: "Cancelled" };
const statusColors = { scheduled: "cyan", in_progress: "purple", completed: "green", cancelled: "red" };

export default function AdminSchedule() {
  const [games, setGames] = useState([]);
  const [loading, setLoading] = useState(true);
  const [selectedDate, setSelectedDate] = useState(new Date());
  const [editDialog, setEditDialog] = useState(false);
  const [editingGame, setEditingGame] = useState(null);
  const [editForm, setEditForm] = useState({});
  const [saving, setSaving] = useState(false);

  useEffect(() => {
    loadGames();
  }, [selectedDate]);

  const loadGames = async () => {
    try {
      setLoading(true);
      const data = await esp32Api.getGamesByDate(format(selectedDate, "yyyy-MM-dd"));
      setGames(Array.isArray(data) ? data : []);
    } catch (error) {
      console.error("Games load error:", error);
      toast.error("Failed to load games");
      setGames([]);
    } finally {
      setLoading(false);
    }
  };

  const openEdit = (game) => {
    setEditingGame(game);
    setEditForm({
      time_slot: game.time_slot || "",
      skill_level: game.skill_level || "beginner",
      status: game.status || "scheduled",
      table_number: game.table_number || "",
    });
    setEditDialog(true);
  };

  const handleSave = async () => {
    if (!editingGame?.id) return;

    try {
      setSaving(true);
      await esp32Api.updateGame(editingGame.id, {
        ...editForm,
        table_number: Number(editForm.table_number) || undefined,
      });
      toast.success("Game updated");
      setEditDialog(false);
      loadGames();
    } catch (error) {
      console.error("Game update error:", error);
      toast.error("Failed to update the game");
    } finally {
      setSaving(false);
    }
  };

  const handleDelete = async (id) => {
    try {
      await esp32Api.deleteGame(id);
      toast.success("Game deleted");
      loadGames();
    } catch (error) {
      console.error("Game delete error:", error);
      toast.error("Failed to delete the game");
    }
  };

  const handleStatusChange = async (game, newStatus) => {
    try {
      await esp32Api.updateGame(game.id, { status: newStatus });
      toast.success("Status updated");
      loadGames();
    } catch (error) {
      console.error("Game status error:", error);
      toast.error("Failed to update the status");
    }
  };

  return (
    <div className="max-w-5xl mx-auto space-y-6">
      <div>
        <h1 className="text-2xl font-bold text-white">Manage Schedule</h1>
        <p className="text-gray-400 mt-1">Edit and manage games</p>
      </div>

      <div className="flex items-center gap-4">
        <Popover>
          <PopoverTrigger asChild>
            <Button variant="outline" className="bg-gray-800 border-gray-700 text-white hover:bg-gray-700">
              <CalendarIcon className="ml-2 h-4 w-4 text-cyan-400" />
              {format(selectedDate, "dd/MM/yyyy")}
            </Button>
          </PopoverTrigger>
          <PopoverContent className="w-auto p-0 bg-gray-900 border-gray-700">
            <Calendar mode="single" selected={selectedDate} onSelect={(d) => d && setSelectedDate(d)} />
          </PopoverContent>
        </Popover>

        <span className="text-gray-400 text-sm">{games.length} games</span>
      </div>

      {loading ? (
        <div className="flex items-center justify-center h-32">
          <Loader2 className="animate-spin text-cyan-400" size={24} />
        </div>
      ) : games.length === 0 ? (
        <div className="glass-card rounded-2xl p-12 border border-white/5 text-center">
          <CalendarIcon size={48} className="mx-auto mb-3 text-gray-600" />
          <p className="text-gray-500">No games on this date</p>
        </div>
      ) : (
        <div className="space-y-3">
          {games.map((game, i) => (
            <motion.div
              key={game.id}
              initial={{ opacity: 0, y: 10 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ delay: i * 0.03 }}
              className="glass-card rounded-xl p-4 border border-white/5"
            >
              <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
                <div className="flex items-center gap-4">
                  <div className="w-12 h-12 rounded-xl bg-gradient-to-br from-cyan-500/20 to-purple-500/20 flex items-center justify-center">
                    <Clock size={20} className="text-cyan-400" />
                  </div>

                  <div>
                    <div className="flex items-center gap-2">
                      <span className="font-bold text-white">{game.time_slot || "—"}</span>
                      <NeonBadge color={statusColors[game.status] || "cyan"}>{statusLabels[game.status] || game.status}</NeonBadge>
                      {game.table_number && <NeonBadge color="purple">Table #{game.table_number}</NeonBadge>}
                    </div>

                    <div className="flex items-center gap-2 text-sm text-gray-400 mt-1">
                      <User size={12} />
                        <span>{game.player1_email || "—"}</span>
                        <span>vs</span>
                        <span>{game.opponent_name || "Robot"}</span>
                    </div>
                  </div>
                </div>

                <div className="flex items-center gap-2">
                  {game.status === "scheduled" && (
                    <Button variant="outline" size="sm" onClick={() => handleStatusChange(game, "in_progress")} className="border-green-500/30 text-green-400 hover:bg-green-500/10 text-xs">
                      Start
                    </Button>
                  )}

                  {game.status === "in_progress" && (
                    <Button variant="outline" size="sm" onClick={() => handleStatusChange(game, "completed")} className="border-green-500/30 text-green-400 hover:bg-green-500/10 text-xs">
                      Finish
                    </Button>
                  )}

                  <Button variant="ghost" size="icon" onClick={() => openEdit(game)} className="text-gray-400 hover:text-white">
                    <Pencil size={14} />
                  </Button>

                  <Button variant="ghost" size="icon" onClick={() => handleDelete(game.id)} className="text-gray-400 hover:text-red-400">
                    <Trash2 size={14} />
                  </Button>
                </div>
              </div>
            </motion.div>
          ))}
        </div>
      )}

      <Dialog open={editDialog} onOpenChange={setEditDialog}>
        <DialogContent className="bg-gray-900 border-gray-700 text-white max-w-md">
          <DialogHeader>
            <DialogTitle>Edit Game</DialogTitle>
          </DialogHeader>

          <div className="space-y-4">
            <div>
              <Label>Time</Label>
              <Input value={editForm.time_slot || ""} onChange={(e) => setEditForm({ ...editForm, time_slot: e.target.value })} className="bg-gray-800 border-gray-700 text-white mt-1" />
            </div>

            <div>
              <Label>Status</Label>
              <Select value={editForm.status || "scheduled"} onValueChange={(v) => setEditForm({ ...editForm, status: v })}>
                <SelectTrigger className="bg-gray-800 border-gray-700 text-white mt-1"><SelectValue /></SelectTrigger>
                <SelectContent className="bg-gray-900 border-gray-700">
                  {Object.entries(statusLabels).map(([k, v]) => (
                    <SelectItem key={k} value={k} className="text-white focus:bg-gray-800">
                      {v}
                    </SelectItem>
                  ))}
                </SelectContent>
              </Select>
            </div>

            <div>
              <Label>Table number</Label>
              <Input type="number" value={editForm.table_number || ""} onChange={(e) => setEditForm({ ...editForm, table_number: e.target.value })} className="bg-gray-800 border-gray-700 text-white mt-1" />
            </div>
          </div>

          <DialogFooter>
            <Button variant="outline" onClick={() => setEditDialog(false)} className="border-gray-700 text-gray-300">Cancel</Button>
            <Button onClick={handleSave} disabled={saving} className="bg-gradient-to-r from-cyan-600 to-purple-600">
              {saving && <Loader2 className="animate-spin ml-2" size={16} />}
              Update
            </Button>
          </DialogFooter>
        </DialogContent>
      </Dialog>
    </div>
  );
}