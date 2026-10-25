import React, { useState } from "react";
import { useNavigate } from "react-router-dom";
import { Gamepad2, LogIn, UserPlus } from "lucide-react";
import { useAuth } from "@/lib/AuthContext";
import { esp32Api, sha256Upper } from "@/api/esp32";

const tabs = [
  { key: "login", label: "Log in", icon: LogIn },
  { key: "register", label: "Sign up", icon: UserPlus },
];

function InputField({ label, type = "text", value, onChange, placeholder }) {
  return (
    <div>
      <label className="block text-sm text-gray-300 mb-1">{label}</label>
      <input
        type={type}
        value={value}
        onChange={(e) => onChange(e.target.value)}
        placeholder={placeholder || label}
        className="w-full rounded-lg border border-gray-700 bg-gray-800 px-4 py-3 text-white outline-none focus:border-cyan-500"
      />
    </div>
  );
}

export default function Login() {
  const navigate = useNavigate();
  const { login } = useAuth();

  const [activeTab, setActiveTab] = useState("login");
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");

  const [loginForm, setLoginForm] = useState({
    email: "",
    password: "",
  });

  const [registerForm, setRegisterForm] = useState({
    name: "",
    email: "",
    phone: "",
    password: "",
    confirmPassword: "",
  });

  function switchTab(tab) {
    setActiveTab(tab);
    setError("");
  }

  async function handleLogin(e) {
    e.preventDefault();
    setError("");

    if (!loginForm.email || !loginForm.password) {
      setError("Please enter email and password");
      return;
    }

    try {
      setLoading(true);

      const result = await esp32Api.login({
        email: loginForm.email.trim(),
        password: loginForm.password,
      });

      if (!result?.success || !result?.user) {
        setError(result?.message || "Login failed");
        return;
      }

      login(result.user);

      if (result.user.role === "admin") {
        navigate("/admin");
      } else {
        navigate("/");
      }
    } catch (err) {
      console.error("login error:", err);
      setError(err.message || "Login error");
    } finally {
      setLoading(false);
    }
  }

  async function handleRegister(e) {
    e.preventDefault();
    setError("");

    if (
      !registerForm.name ||
      !registerForm.email ||
      !registerForm.phone ||
      !registerForm.password
    ) {
      setError("Please enter name, email, phone and password");
      return;
    }

    if (registerForm.password !== registerForm.confirmPassword) {
      setError("The passwords do not match");
      return;
    }

    try {
      setLoading(true);

      const passwordHash = await sha256Upper(registerForm.password);

      const payload = {
        createdAt: new Date().toISOString(),
        email: registerForm.email.trim(),
        isActive: true,
        name: registerForm.name.trim(),
        password: registerForm.password,
        passwordHash,
        phone: registerForm.phone.trim(),
        role: "user",
      };

      const result = await esp32Api.register(payload);

      if (!result?.success) {
        setError(result?.message || "Sign up failed");
        return;
      }

      const user = result.user || {
        email: payload.email,
        name: payload.name,
        phone: payload.phone,
        role: "user",
        isActive: true,
      };

      login(user);
      navigate("/");
    } catch (err) {
      console.error("register error:", err);
      setError(err.message || "Sign up error");
    } finally {
      setLoading(false);
    }
  }

  return (
    <div
      dir="ltr"
      className="min-h-screen bg-gray-950 text-white flex items-center justify-center px-4 py-10"
    >
      <div className="w-full max-w-xl bg-gray-900 border border-gray-800 rounded-2xl p-6 shadow-xl">
        <div className="text-center mb-6">
          <div className="mx-auto mb-3 w-14 h-14 rounded-2xl bg-cyan-500/10 border border-cyan-400/30 flex items-center justify-center">
            <Gamepad2 className="text-cyan-400" size={30} />
          </div>

          <h1 className="text-3xl font-bold">AirHockey Robot</h1>
          <p className="text-gray-400 mt-1">Log in or sign up</p>
        </div>

        <div className="grid grid-cols-2 gap-2 mb-6 rounded-xl bg-gray-950 p-1 border border-gray-800">
          {tabs.map((tab) => {
            const Icon = tab.icon;

            return (
              <button
                key={tab.key}
                onClick={() => switchTab(tab.key)}
                className={`py-3 rounded-lg text-sm font-semibold flex items-center justify-center gap-2 transition-all ${
                  activeTab === tab.key
                    ? "bg-gradient-to-r from-cyan-600 to-purple-600 text-white"
                    : "text-gray-400 hover:bg-gray-800"
                }`}
              >
                <Icon size={16} />
                {tab.label}
              </button>
            );
          })}
        </div>

        {activeTab === "login" && (
          <form onSubmit={handleLogin} className="space-y-4">
            <InputField
              label="Email"
              type="email"
              value={loginForm.email}
              onChange={(v) => setLoginForm({ ...loginForm, email: v })}
            />

            <InputField
              label="Password"
              type="password"
              value={loginForm.password}
              onChange={(v) => setLoginForm({ ...loginForm, password: v })}
            />

            {error ? (
              <div className="text-red-400 text-sm text-center">{error}</div>
            ) : null}

            <button
              disabled={loading}
              className="w-full rounded-lg bg-gradient-to-r from-cyan-600 to-purple-600 py-3 font-semibold hover:opacity-90 disabled:opacity-50"
            >
              {loading ? "Logging in..." : "Log in"}
            </button>
          </form>
        )}

        {activeTab === "register" && (
          <form onSubmit={handleRegister} className="space-y-4">
            <InputField
              label="Name"
              value={registerForm.name}
              onChange={(v) => setRegisterForm({ ...registerForm, name: v })}
            />

            <InputField
              label="Email"
              type="email"
              value={registerForm.email}
              onChange={(v) => setRegisterForm({ ...registerForm, email: v })}
            />

            <InputField
              label="Phone"
              value={registerForm.phone}
              onChange={(v) => setRegisterForm({ ...registerForm, phone: v })}
            />

            <InputField
              label="Password"
              type="password"
              value={registerForm.password}
              onChange={(v) =>
                setRegisterForm({ ...registerForm, password: v })
              }
            />

            <InputField
              label="Confirm password"
              type="password"
              value={registerForm.confirmPassword}
              onChange={(v) =>
                setRegisterForm({ ...registerForm, confirmPassword: v })
              }
            />

            {error ? (
              <div className="text-red-400 text-sm text-center">{error}</div>
            ) : null}

            <button
              disabled={loading}
              className="w-full rounded-lg bg-gradient-to-r from-cyan-600 to-purple-600 py-3 font-semibold hover:opacity-90 disabled:opacity-50"
            >
              {loading ? "Signing up..." : "Sign up"}
            </button>
          </form>
        )}
      </div>
    </div>
  );
}