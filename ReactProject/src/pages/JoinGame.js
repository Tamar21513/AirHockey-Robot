import React, { useState, useEffect } from "react";
import { useNavigate } from "react-router-dom";
import {
  Calendar as CalendarIcon,
  CreditCard,
  Loader2,
  Check,
  Banknote,
  Smartphone,
  Globe,
  Gift,
  Ticket,
  Copy,
} from "lucide-react";
import { Button } from "@/components/ui/button";
import { Calendar } from "@/components/ui/calendar";
import { Popover, PopoverContent, PopoverTrigger } from "@/components/ui/popover";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";
import { format } from "date-fns";
import { motion, AnimatePresence } from "framer-motion";
import { toast } from "sonner";
import { useAuth } from "@/lib/AuthContext";
import { esp32Api } from "@/api/esp32";

const paymentMethods = [
  { value: "credit_card", label: "Credit card", icon: CreditCard },
  { value: "cash", label: "Cash", icon: Banknote },
  { value: "bit", label: "Bit", icon: Smartphone },
  { value: "paypal", label: "PayPal", icon: Globe },
];

const timeSlots = [
  "09:00",
  "10:00",
  "11:00",
  "12:00",
  "13:00",
  "14:00",
  "15:00",
  "16:00",
  "17:00",
  "18:00",
  "19:00",
  "20:00",
  "21:00",
  "22:00",
];

function getBonusId(bonus) {
  return bonus?.id || bonus?._id || bonus?.bonus_id || bonus?.document_id || "";
}

function getBonusTitle(bonus) {
  return bonus?.description || bonus?.title || bonus?.type || "Available bonus";
}

function createGameCode() {
  return String(Math.floor(100000 + Math.random() * 900000));
}

export default function JoinGame() {
  const navigate = useNavigate();
  const { currentUser } = useAuth();

  const [step, setStep] = useState(1);
  const [loading, setLoading] = useState(false);
  const [showSuccess, setShowSuccess] = useState(false);
  const [gameCode, setGameCode] = useState("");

  const [form, setForm] = useState({
    date: null,
    time_slot: "",
    payment_source: "",
    payment_method: "",
    bonus_id: "",
  });

  const [userSub, setUserSub] = useState(null);
  const [userBonuses, setUserBonuses] = useState([]);

  useEffect(() => {
    loadUserSubscription();
    loadUserBonuses();
  }, [currentUser]);

  async function loadUserSubscription() {
    if (!currentUser?.email) return;

    try {
      const data = await esp32Api.getUserSubscription(currentUser.email);
      if (data && Number(data.games_remaining || 0) > 0) {
        setUserSub(data);
      } else {
        setUserSub(null);
      }
    } catch {
      setUserSub(null);
    }
  }

  async function loadUserBonuses() {
    if (!currentUser?.email) return;

    try {
      const data = await esp32Api.getUserBonuses(currentUser.email);
      const bonuses = Array.isArray(data) ? data : data?.bonuses || [];
      setUserBonuses(bonuses.filter((b) => !b?.is_claimed));
    } catch {
      setUserBonuses([]);
    }
  }

  function resetPayment(newPaymentSource) {
    setForm({
      ...form,
      payment_source: newPaymentSource,
      payment_method: newPaymentSource === "regular" ? "" : newPaymentSource,
      bonus_id: "",
    });
  }

  const handleSubmit = async () => {
    if (!currentUser?.email) {
      toast.error("No user is logged in");
      return;
    }

    if (!form.date || !form.time_slot) {
      toast.error("Please choose a date and time");
      return;
    }

    if (!form.payment_source) {
      toast.error("Please choose a payment option");
      return;
    }

    if (form.payment_source === "regular" && !form.payment_method) {
      toast.error("Please choose a payment method");
      return;
    }

    if (form.payment_source === "bonus" && !form.bonus_id) {
      toast.error("Please choose a bonus");
      return;
    }

    setLoading(true);

    try {
      const newGameCode = createGameCode();
      const result = await esp32Api.createGame({
        game_code: newGameCode,
        date: format(form.date, "yyyy-MM-dd"),
        time_slot: form.time_slot,

        payment_source: form.payment_source,
        payment_method: form.payment_method,
        bonus_id: form.bonus_id || "",

        player_email: currentUser.email,
        player_name: currentUser.name || "Player",
        opponent_type: "robot",
        opponent_name: "Robot",

        skill_level: 0,
        game_mode: "",
        status: "scheduled",
      });

      if (result?.success) {
        setGameCode(result?.game_code || result?.game?.game_code || newGameCode);
        setShowSuccess(true);

        setTimeout(() => {
          navigate("/");
        }, 6500);
      } else {
        toast.error(result?.message || "Error booking the game");
      }
    } catch (err) {
      console.error(err);
      toast.error(err.message || "ESP32 is not available");
    }

    setLoading(false);
  };

  const copyGameCode = async () => {
    try {
      await navigator.clipboard.writeText(gameCode);
      toast.success("Game code copied");
    } catch {
      toast.error("Could not copy the code");
    }
  };

  const disableSubmit =
    loading ||
    !form.payment_source ||
    (form.payment_source === "regular" && !form.payment_method) ||
    (form.payment_source === "bonus" && !form.bonus_id);

  return (
    <div className="max-w-2xl mx-auto space-y-6 relative">
      {showSuccess && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/70 backdrop-blur-sm overflow-hidden">
          <div className="absolute inset-0 pointer-events-none">
            {Array.from({ length: 70 }).map((_, index) => (
              <span
                key={index}
                className="absolute text-2xl animate-confetti"
                style={{
                  left: `${Math.random() * 100}%`,
                  animationDelay: `${Math.random() * 1.4}s`,
                  animationDuration: `${2 + Math.random() * 1.8}s`,
                }}
              >
                {index % 5 === 0
                  ? "🎉"
                  : index % 5 === 1
                  ? "✨"
                  : index % 5 === 2
                  ? "🎊"
                  : index % 5 === 3
                  ? "💠"
                  : "⭐"}
              </span>
            ))}
          </div>

          <motion.div
            initial={{ opacity: 0, scale: 0.8, y: 20 }}
            animate={{ opacity: 1, scale: 1, y: 0 }}
            className="relative z-10 w-[90%] max-w-md rounded-3xl border border-cyan-400/30 bg-gray-950 p-8 text-center shadow-2xl shadow-cyan-500/20"
          >
            <div className="mx-auto mb-5 flex h-16 w-16 items-center justify-center rounded-full bg-green-500/15 border border-green-400/40">
              <Check className="text-green-400" size={34} />
            </div>

            <h2 className="text-2xl font-bold text-white">The booking was added successfully</h2>

            <div className="mt-5 rounded-2xl border border-cyan-500/30 bg-cyan-500/10 p-5">
              <p className="text-sm text-cyan-200 mb-2">Game code</p>
              <div className="flex items-center justify-center gap-3">
                <p className="text-4xl font-black tracking-[0.25em] text-cyan-300 ltr" dir="ltr">
                  {gameCode}
                </p>
                <button onClick={copyGameCode} className="rounded-lg border border-cyan-500/40 p-2 text-cyan-300 hover:bg-cyan-500/20">
                  <Copy size={18} />
                </button>
              </div>
              <p className="mt-3 text-xs text-gray-400">
                Use this code to start the game from the login screen.
              </p>
            </div>

            <p className="mt-4 text-gray-400">You will be taken to the home screen shortly</p>
          </motion.div>
        </div>
      )}

      <style>{`
        @keyframes confettiFall {
          0% { transform: translateY(-20vh) rotate(0deg); opacity: 1; }
          100% { transform: translateY(120vh) rotate(720deg); opacity: 0; }
        }
        .animate-confetti {
          top: -10vh;
          animation-name: confettiFall;
          animation-timing-function: linear;
          animation-fill-mode: forwards;
        }
      `}</style>

      <div>
        <h1 className="text-2xl font-bold text-white">Book a Game</h1>
        <p className="text-gray-400 mt-1">Choose a date, time and payment option</p>
      </div>

      <div className="flex items-center gap-2">
        {[1, 2].map((s) => (
          <React.Fragment key={s}>
            <div
              className={`w-10 h-10 rounded-full flex items-center justify-center font-bold text-sm transition-all ${
                step >= s
                  ? "bg-gradient-to-br from-cyan-500 to-purple-600 text-white"
                  : "bg-gray-800 text-gray-500"
              }`}
            >
              {step > s ? <Check size={16} /> : s}
            </div>
            {s < 2 && <div className={`flex-1 h-0.5 ${step > s ? "bg-cyan-500" : "bg-gray-800"}`} />}
          </React.Fragment>
        ))}
      </div>

      <AnimatePresence mode="wait">
        {step === 1 && (
          <motion.div
            key="step1"
            initial={{ opacity: 0, x: 20 }}
            animate={{ opacity: 1, x: 0 }}
            exit={{ opacity: 0, x: -20 }}
            className="glass-card rounded-2xl p-6 border border-white/10 space-y-5"
          >
            <h2 className="text-lg font-semibold">Choose date and time</h2>

            <div className="space-y-4">
              <Popover>
                <PopoverTrigger asChild>
                  <Button variant="outline" className="w-full justify-start bg-gray-800 border-gray-700 text-white hover:bg-gray-700">
                    <CalendarIcon className="ml-2 h-4 w-4 text-cyan-400" />
                    {form.date ? format(form.date, "dd/MM/yyyy") : "Choose date"}
                  </Button>
                </PopoverTrigger>

                <PopoverContent className="w-auto p-0 bg-gray-900 border-gray-700">
                  <Calendar
                    mode="single"
                    selected={form.date}
                    onSelect={(d) => setForm({ ...form, date: d })}
                    disabled={(date) => date < new Date(new Date().setHours(0, 0, 0, 0))}
                  />
                </PopoverContent>
              </Popover>

              <Select value={form.time_slot} onValueChange={(v) => setForm({ ...form, time_slot: v })}>
                <SelectTrigger className="bg-gray-800 border-gray-700 text-white">
                  <SelectValue placeholder="Choose time" />
                </SelectTrigger>

                <SelectContent className="bg-gray-900 border-gray-700">
                  {timeSlots.map((t) => (
                    <SelectItem key={t} value={t} className="text-white focus:bg-gray-800">
                      {t}
                    </SelectItem>
                  ))}
                </SelectContent>
              </Select>
            </div>

            <Button className="w-full bg-gradient-to-r from-cyan-600 to-purple-600 hover:opacity-90" disabled={!form.date || !form.time_slot} onClick={() => setStep(2)}>
              Continue to payment
            </Button>
          </motion.div>
        )}

        {step === 2 && (
          <motion.div
            key="step2"
            initial={{ opacity: 0, x: 20 }}
            animate={{ opacity: 1, x: 0 }}
            exit={{ opacity: 0, x: -20 }}
            className="glass-card rounded-2xl p-6 border border-white/10 space-y-5"
          >
            <h2 className="text-lg font-semibold">Payment option</h2>

            {userSub && (
              <button
                onClick={() => resetPayment("subscription")}
                className={`w-full p-4 rounded-xl border text-right transition-all ${
                  form.payment_source === "subscription"
                    ? "border-green-500 bg-green-500/10"
                    : "border-gray-700 bg-gray-800/50 hover:border-gray-600"
                }`}
              >
                <div className="flex items-center justify-between gap-3">
                  <div>
                    <p className="text-green-400 font-semibold">Use active subscription</p>
                    <p className="text-green-300 text-sm mt-1">
                      {userSub.subscription_name || "Subscription"} • Remaining: {userSub.games_remaining} games
                    </p>
                  </div>
                  <Ticket className="text-green-400" size={22} />
                </div>
              </button>
            )}

            {userBonuses.length > 0 && (
              <div className="space-y-3">
                <h3 className="text-sm font-semibold text-gray-300">Available bonuses</h3>

                {userBonuses.map((bonus, index) => {
                  const bonusId = getBonusId(bonus) || String(index);

                  return (
                    <button
                      key={bonusId}
                      onClick={() =>
                        setForm({
                          ...form,
                          payment_source: "bonus",
                          payment_method: "bonus",
                          bonus_id: bonusId,
                        })
                      }
                      className={`w-full p-4 rounded-xl border text-right transition-all ${
                        form.payment_source === "bonus" && form.bonus_id === bonusId
                          ? "border-purple-500 bg-purple-500/10"
                          : "border-gray-700 bg-gray-800/50 hover:border-gray-600"
                      }`}
                    >
                      <div className="flex items-center justify-between gap-3">
                        <div>
                          <p className="text-purple-400 font-semibold">{getBonusTitle(bonus)}</p>
                          {bonus?.amount !== undefined && <p className="text-gray-400 text-sm mt-1">Bonus value: ₪{bonus.amount}</p>}
                        </div>
                        <Gift className="text-purple-400" size={22} />
                      </div>
                    </button>
                  );
                })}
              </div>
            )}

            <div className="space-y-3">
              <h3 className="text-sm font-semibold text-gray-300">Regular payment</h3>

              <button
                onClick={() => resetPayment("regular")}
                className={`w-full p-4 rounded-xl border text-right transition-all ${
                  form.payment_source === "regular"
                    ? "border-cyan-500 bg-cyan-500/10"
                    : "border-gray-700 bg-gray-800/50 hover:border-gray-600"
                }`}
              >
                <p className="text-cyan-400 font-semibold">One-time payment</p>
                <p className="text-gray-400 text-sm mt-1">Payment for one game</p>
              </button>
            </div>

            {form.payment_source === "regular" && (
              <div className="grid grid-cols-2 gap-3">
                {paymentMethods.map((method) => (
                  <button
                    key={method.value}
                    onClick={() => setForm({ ...form, payment_method: method.value })}
                    className={`flex items-center gap-3 p-4 rounded-xl border transition-all ${
                      form.payment_method === method.value
                        ? "border-cyan-500 bg-cyan-500/10"
                        : "border-gray-700 bg-gray-800/50 hover:border-gray-600"
                    }`}
                  >
                    <method.icon size={20} className="text-gray-400" />
                    <span className="text-sm">{method.label}</span>
                  </button>
                ))}
              </div>
            )}

            <div className="p-4 rounded-xl bg-gray-800/50 border border-gray-700 space-y-2">
              <h3 className="text-sm font-semibold text-gray-300">Booking summary</h3>

              <div className="flex justify-between text-sm">
                <span className="text-gray-400">Date</span>
                <span>{form.date ? format(form.date, "dd/MM/yyyy") : ""}</span>
              </div>

              <div className="flex justify-between text-sm">
                <span className="text-gray-400">Time</span>
                <span>{form.time_slot}</span>
              </div>

              <div className="flex justify-between text-sm">
                <span className="text-gray-400">Payment option</span>
                <span>
                  {form.payment_source === "subscription" && "Subscription"}
                  {form.payment_source === "bonus" && "Bonus"}
                  {form.payment_source === "regular" && "Regular payment"}
                  {!form.payment_source && "Not selected"}
                </span>
              </div>

              {form.payment_source === "regular" && (
                <div className="flex justify-between text-sm pt-2 border-t border-gray-700">
                  <span className="text-gray-400">Price</span>
                  <span className="text-cyan-400 font-bold">₪35</span>
                </div>
              )}

              {form.payment_source === "subscription" && (
                <div className="flex justify-between text-sm pt-2 border-t border-gray-700">
                  <span className="text-gray-400">Charge</span>
                  <span className="text-green-400 font-bold">One game will be deducted from the subscription</span>
                </div>
              )}

              {form.payment_source === "bonus" && (
                <div className="flex justify-between text-sm pt-2 border-t border-gray-700">
                  <span className="text-gray-400">Charge</span>
                  <span className="text-purple-400 font-bold">Use bonus</span>
                </div>
              )}
            </div>

            <div className="flex gap-3">
              <Button variant="outline" onClick={() => setStep(1)} className="border-gray-700 text-gray-300 hover:bg-gray-800">
                Back
              </Button>

              <Button className="flex-1 bg-gradient-to-r from-cyan-600 to-purple-600 hover:opacity-90" disabled={disableSubmit} onClick={handleSubmit}>
                {loading ? <Loader2 className="animate-spin ml-2" size={16} /> : null}
                Confirm booking
              </Button>
            </div>
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
}
