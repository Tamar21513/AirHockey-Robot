import React, { useState, useEffect } from "react";
import { Plus, Pencil, Trash2, Loader2 } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { Textarea } from "@/components/ui/textarea";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";
import { Switch } from "@/components/ui/switch";
import { Dialog, DialogContent, DialogHeader, DialogTitle, DialogFooter } from "@/components/ui/dialog";
import { toast } from "sonner";
import { motion } from "framer-motion";
import NeonBadge from "@/components/ui/NeonBadge";
import { esp32Api } from "@/api/esp32";

const typeLabels = { monthly: "Monthly", quarterly: "Quarterly", yearly: "Yearly", one_time: "One-time" };
const typeColors = { monthly: "cyan", quarterly: "purple", yearly: "green", one_time: "orange" };
const emptyForm = { name: "", type: "monthly", price: "", games_included: "", description: "", benefits: "", is_active: true };

export default function AdminSubscriptions() {
  const [subs, setSubs] = useState([]);
  const [loading, setLoading] = useState(true);
  const [dialogOpen, setDialogOpen] = useState(false);
  const [editing, setEditing] = useState(null);
  const [form, setForm] = useState(emptyForm);
  const [saving, setSaving] = useState(false);

  useEffect(() => {
    loadData();
  }, []);

  const loadData = async () => {
    try {
      setLoading(true);
      const data = await esp32Api.getSubscriptions();
      setSubs(Array.isArray(data) ? data : []);
    } catch (error) {
      console.error("Subscriptions admin load error:", error);
      toast.error("Error loading subscriptions");
      setSubs([]);
    } finally {
      setLoading(false);
    }
  };

  const openNew = () => {
    setEditing(null);
    setForm(emptyForm);
    setDialogOpen(true);
  };

  const openEdit = (sub) => {
    setEditing(sub);
    setForm({
      name: sub.name || "",
      type: sub.type || "monthly",
      price: sub.price || "",
      games_included: sub.games_included || "",
      description: sub.description || "",
      benefits: sub.benefits || "",
      is_active: sub.is_active !== false,
    });
    setDialogOpen(true);
  };

  const handleSave = async () => {
    try {
      setSaving(true);

      const data = {
        ...form,
        price: Number(form.price),
        games_included: Number(form.games_included) || 0,
      };

      if (editing) {
        await esp32Api.updateSubscription(editing.id, data);
        toast.success("Subscription updated successfully");
      } else {
        await esp32Api.createSubscription(data);
        toast.success("Subscription created successfully");
      }

      setDialogOpen(false);
      loadData();
    } catch (error) {
      console.error("Subscription save error:", error);
      toast.error("Failed to save the subscription");
    } finally {
      setSaving(false);
    }
  };

  const handleDelete = async (id) => {
    try {
      await esp32Api.deleteSubscription(id);
      toast.success("Subscription deleted");
      loadData();
    } catch (error) {
      console.error("Subscription delete error:", error);
      toast.error("Failed to delete the subscription");
    }
  };

  if (loading) return <div className="flex items-center justify-center h-64"><Loader2 className="animate-spin text-cyan-400" size={32} /></div>;

  return (
    <div className="max-w-5xl mx-auto space-y-6">
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-2xl font-bold text-white">Manage Subscriptions</h1>
          <p className="text-gray-400 mt-1">Create, edit and delete subscription types</p>
        </div>
        <Button onClick={openNew} className="bg-gradient-to-r from-cyan-600 to-purple-600 hover:opacity-90">
          <Plus size={16} className="ml-2" />
          New Subscription
        </Button>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
        {subs.map((sub, i) => (
          <motion.div
            key={sub.id}
            initial={{ opacity: 0, y: 10 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ delay: i * 0.05 }}
            className={`glass-card rounded-2xl p-5 border ${sub.is_active !== false ? "border-white/10" : "border-red-500/20 opacity-60"}`}
          >
            <div className="flex items-start justify-between">
              <div>
                <div className="flex items-center gap-2 mb-1">
                  <NeonBadge color={typeColors[sub.type] || "cyan"}>{typeLabels[sub.type] || sub.type}</NeonBadge>
                  {sub.is_active === false && <NeonBadge color="red">Inactive</NeonBadge>}
                </div>

                <h3 className="text-lg font-bold text-white mt-2">{sub.name}</h3>
                <p className="text-2xl font-black text-cyan-400 mt-1">₪{sub.price}</p>
                {sub.games_included > 0 && <p className="text-sm text-gray-400 mt-1">{sub.games_included} games</p>}
                {sub.description && <p className="text-sm text-gray-500 mt-2">{sub.description}</p>}
              </div>

              <div className="flex gap-1">
                <Button variant="ghost" size="icon" onClick={() => openEdit(sub)} className="text-gray-400 hover:text-white hover:bg-white/10">
                  <Pencil size={14} />
                </Button>
                <Button variant="ghost" size="icon" onClick={() => handleDelete(sub.id)} className="text-gray-400 hover:text-red-400 hover:bg-red-500/10">
                  <Trash2 size={14} />
                </Button>
              </div>
            </div>
          </motion.div>
        ))}
      </div>

      <Dialog open={dialogOpen} onOpenChange={setDialogOpen}>
        <DialogContent className="bg-gray-900 border-gray-700 text-white max-w-md">
          <DialogHeader>
            <DialogTitle>{editing ? "Edit Subscription" : "New Subscription"}</DialogTitle>
          </DialogHeader>

          <div className="space-y-4">
            <div>
              <Label>Subscription name</Label>
              <Input value={form.name} onChange={(e) => setForm({ ...form, name: e.target.value })} className="bg-gray-800 border-gray-700 text-white mt-1" />
            </div>

            <div>
              <Label>Type</Label>
              <Select value={form.type} onValueChange={(v) => setForm({ ...form, type: v })}>
                <SelectTrigger className="bg-gray-800 border-gray-700 text-white mt-1"><SelectValue /></SelectTrigger>
                <SelectContent className="bg-gray-900 border-gray-700">
                  {Object.entries(typeLabels).map(([k, v]) => (
                    <SelectItem key={k} value={k} className="text-white focus:bg-gray-800">
                      {v}
                    </SelectItem>
                  ))}
                </SelectContent>
              </Select>
            </div>

            <div className="grid grid-cols-2 gap-3">
              <div>
                <Label>Price (₪)</Label>
                <Input type="number" value={form.price} onChange={(e) => setForm({ ...form, price: e.target.value })} className="bg-gray-800 border-gray-700 text-white mt-1" />
              </div>
              <div>
                <Label>Games included</Label>
                <Input type="number" value={form.games_included} onChange={(e) => setForm({ ...form, games_included: e.target.value })} className="bg-gray-800 border-gray-700 text-white mt-1" />
              </div>
            </div>

            <div>
              <Label>Description</Label>
              <Textarea value={form.description} onChange={(e) => setForm({ ...form, description: e.target.value })} className="bg-gray-800 border-gray-700 text-white mt-1" rows={2} />
            </div>

            <div>
              <Label>Benefits (comma separated)</Label>
              <Input value={form.benefits} onChange={(e) => setForm({ ...form, benefits: e.target.value })} className="bg-gray-800 border-gray-700 text-white mt-1" placeholder="Benefit 1, Benefit 2" />
            </div>

            <div className="flex items-center justify-between">
              <Label>Active subscription</Label>
              <Switch checked={form.is_active} onCheckedChange={(v) => setForm({ ...form, is_active: v })} />
            </div>
          </div>

          <DialogFooter>
            <Button variant="outline" onClick={() => setDialogOpen(false)} className="border-gray-700 text-gray-300">
              Cancel
            </Button>
            <Button onClick={handleSave} disabled={!form.name || !form.price || saving} className="bg-gradient-to-r from-cyan-600 to-purple-600">
              {saving ? <Loader2 className="animate-spin ml-2" size={16} /> : null}
              {editing ? "Update" : "Create"}
            </Button>
          </DialogFooter>
        </DialogContent>
      </Dialog>
    </div>
  );
}