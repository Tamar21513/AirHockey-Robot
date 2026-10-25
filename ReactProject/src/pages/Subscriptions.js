import React, { useState, useEffect } from "react";
import { Gift, Check, Loader2, Star, CreditCard, Banknote, Smartphone, Globe } from "lucide-react";
import { Button } from "@/components/ui/button";
import { motion } from "framer-motion";
import { toast } from "sonner";
import NeonBadge from "@/components/ui/NeonBadge";
import { useAuth } from "@/lib/AuthContext";
import { esp32Api } from "@/api/esp32";

const paymentMethods = [
  { value: "credit_card", label: "Credit card", icon: CreditCard },
  { value: "cash", label: "Cash", icon: Banknote },
  { value: "bit", label: "Bit", icon: Smartphone },
  { value: "paypal", label: "PayPal", icon: Globe },
];

const typeLabels = { monthly: "Monthly", quarterly: "Quarterly", yearly: "Yearly", one_time: "One-time" };
const typeColors = { monthly: "cyan", quarterly: "purple", yearly: "green", one_time: "orange" };

export default function Subscriptions() {
  const { currentUser } = useAuth();

  const [subs, setSubs] = useState([]);
  const [userSub, setUserSub] = useState(null);
  const [loading, setLoading] = useState(true);
  const [selectedSub, setSelectedSub] = useState(null);
  const [paymentMethod, setPaymentMethod] = useState("");
  const [submitting, setSubmitting] = useState(false);

  useEffect(() => {
    loadData();
  }, [currentUser]);

  async function loadData() {
    try {
      setLoading(true);

      const subsData = await esp32Api.getSubscriptions();
      setSubs(Array.isArray(subsData) ? subsData : []);

      if (currentUser?.email) {
        try {
          const userSubData = await esp32Api.getUserSubscription(currentUser.email);
          setUserSub(userSubData || null);
        } catch {
          setUserSub(null);
        }
      } else {
        setUserSub(null);
      }
    } catch (error) {
      console.error("loadData error:", error);
      toast.error("Error loading data from the ESP32");
      setSubs([]);
      setUserSub(null);
    } finally {
      setLoading(false);
    }
  }

  const handleSubscribe = async () => {
    if (!selectedSub || !paymentMethod || !currentUser?.email) return;

    try {
      setSubmitting(true);

      const sub = subs.find((s) => s.id === selectedSub);
      if (!sub) {
        toast.error("No valid subscription selected");
        return;
      }

      const now = new Date();
      const endDate = new Date();

      if (sub.type === "monthly") endDate.setMonth(endDate.getMonth() + 1);
      else if (sub.type === "quarterly") endDate.setMonth(endDate.getMonth() + 3);
      else if (sub.type === "yearly") endDate.setFullYear(endDate.getFullYear() + 1);
      else endDate.setDate(endDate.getDate() + 1);

      await esp32Api.subscribe({
        user_email: currentUser.email,
        subscription_id: sub.id,
        subscription_name: sub.name,
        type: sub.type,
        price: sub.price,
        games_remaining: sub.games_included || 0,
        payment_method: paymentMethod,
        start_date: now.toISOString().split("T")[0],
        end_date: endDate.toISOString().split("T")[0],
        status: "active",
      });

      toast.success("You subscribed successfully!");
      setSelectedSub(null);
      setPaymentMethod("");
      await loadData();
    } catch (error) {
      console.error("handleSubscribe error:", error);
      toast.error("Error subscribing");
    } finally {
      setSubmitting(false);
    }
  };

  if (loading) {
    return (
      <div className="flex items-center justify-center h-64">
        <Loader2 className="animate-spin text-cyan-400" size={32} />
      </div>
    );
  }

  return (
    <div className="max-w-5xl mx-auto space-y-6">
      <div>
        <h1 className="text-2xl font-bold text-white">Subscriptions</h1>
        <p className="text-gray-400 mt-1">Choose the subscription that suits you</p>
      </div>

      {userSub && (
        <motion.div
          initial={{ opacity: 0, y: 10 }}
          animate={{ opacity: 1, y: 0 }}
          className="glass-card rounded-2xl p-6 border border-green-500/30 bg-gradient-to-br from-green-500/10 to-transparent"
        >
          <div className="flex items-center gap-2 mb-2">
            <Star size={18} className="text-green-400" />
            <h2 className="font-semibold text-green-400">Your active subscription</h2>
          </div>
          <p className="text-white font-bold text-lg">{userSub.subscription_name}</p>
          <p className="text-gray-400 text-sm mt-1">
            Remaining: {userSub.games_remaining} games • Valid until {userSub.end_date}
          </p>
        </motion.div>
      )}

      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
        {subs.map((sub, i) => (
          <motion.div
            key={sub.id}
            initial={{ opacity: 0, y: 20 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ delay: i * 0.05 }}
            onClick={() => setSelectedSub(selectedSub === sub.id ? null : sub.id)}
            className={`glass-card rounded-2xl p-6 border cursor-pointer transition-all duration-300 ${
              selectedSub === sub.id ? "border-cyan-500 ring-1 ring-cyan-500/30" : "border-white/5 hover:border-white/15"
            }`}
          >
            <NeonBadge color={typeColors[sub.type] || "cyan"}>{typeLabels[sub.type] || sub.type}</NeonBadge>

            <h3 className="text-xl font-bold text-white mt-3">{sub.name}</h3>

            <div className="mt-2">
              <span className="text-3xl font-bold text-white">₪{sub.price}</span>
              <span className="text-gray-400 text-sm">/{typeLabels[sub.type] || sub.type}</span>
            </div>

            {sub.games_included ? <p className="text-gray-400 text-sm mt-2">{sub.games_included} games included</p> : null}
            {sub.description ? <p className="text-gray-500 text-sm mt-2">{sub.description}</p> : null}

            {sub.benefits && (
              <div className="mt-3 space-y-1">
                {String(sub.benefits).split(",").map((b, j) => (
                  <div key={j} className="flex items-center gap-2 text-sm text-gray-300">
                    <Check size={14} className="text-green-400" />
                    <span>{b.trim()}</span>
                  </div>
                ))}
              </div>
            )}
          </motion.div>
        ))}

        {subs.length === 0 && (
          <div className="col-span-full text-center py-12 text-gray-500">
            <Gift size={48} className="mx-auto mb-3 opacity-50" />
            <p>No subscriptions available right now</p>
          </div>
        )}
      </div>

      {selectedSub && (
        <motion.div
          initial={{ opacity: 0, y: 10 }}
          animate={{ opacity: 1, y: 0 }}
          className="glass-card rounded-2xl p-6 border border-white/10 space-y-4"
        >
          <h3 className="font-semibold text-white">Payment method</h3>

          <div className="grid grid-cols-2 sm:grid-cols-4 gap-3">
            {paymentMethods.map((m) => (
              <button
                key={m.value}
                onClick={() => setPaymentMethod(m.value)}
                className={`flex items-center gap-2 p-3 rounded-xl border transition-all ${
                  paymentMethod === m.value ? "border-cyan-500 bg-cyan-500/10" : "border-gray-700 hover:border-gray-600"
                }`}
              >
                <m.icon size={16} className="text-gray-400" />
                <span className="text-sm">{m.label}</span>
              </button>
            ))}
          </div>

          <Button
            className="w-full bg-gradient-to-r from-cyan-600 to-purple-600 hover:opacity-90"
            disabled={!paymentMethod || submitting}
            onClick={handleSubscribe}
          >
            {submitting ? <Loader2 className="animate-spin ml-2" size={16} /> : null}
            Confirm and subscribe
          </Button>
        </motion.div>
      )}
    </div>
  );
}