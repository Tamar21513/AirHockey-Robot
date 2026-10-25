import React from "react";
import { AuthProvider, useAuth } from "./lib/AuthContext";
import { Routes, Route, Navigate } from "react-router-dom";

import Layout from "./Layout";
import Login from "./pages/Login";
import Home from "./pages/Home";
import JoinGame from "./pages/JoinGame";
import Subscriptions from "./pages/Subscriptions";
import UserHistory from "./pages/UserHistory";
import Leaderboard from "./pages/Leaderboard";
import LiveGame from "./pages/LiveGame";
import AdminDashboard from "./pages/AdminDashboard";
import AdminSubscriptions from "./pages/AdminSubscriptions";
import AdminSchedule from "./pages/AdminSchedule";
import AdminPlayers from "./pages/AdminPlayers";
import AdminBonuses from "./pages/AdminBonuses";

function Page({ component: Component, currentPath }) {
  return (
    <Layout currentPath={currentPath}>
      <Component />
    </Layout>
  );
}

function ProtectedRoute({ children, adminOnly = false, allowGameSession = false }) {
  const { currentUser, activeGame } = useAuth();

  if (!currentUser && !(allowGameSession && activeGame)) {
    return <Navigate to="/login" replace />;
  }

  if (adminOnly && currentUser?.role !== "admin") {
    return <Navigate to="/" replace />;
  }

  return children;
}

function AppRoutes() {
  const { currentUser } = useAuth();

  return (
    <Routes>
      <Route
        path="/login"
        element={currentUser ? <Navigate to="/" replace /> : <Login />}
      />

      <Route
        path="/"
        element={
          <ProtectedRoute>
            <Page component={Home} currentPath="/" />
          </ProtectedRoute>
        }
      />

      <Route
        path="/join"
        element={
          <ProtectedRoute>
            <Page component={JoinGame} currentPath="/join" />
          </ProtectedRoute>
        }
      />

      <Route
        path="/subscriptions"
        element={
          <ProtectedRoute>
            <Page component={Subscriptions} currentPath="/subscriptions" />
          </ProtectedRoute>
        }
      />

      <Route
        path="/history"
        element={
          <ProtectedRoute>
            <Page component={UserHistory} currentPath="/history" />
          </ProtectedRoute>
        }
      />

      <Route
        path="/leaderboard"
        element={
          <ProtectedRoute>
            <Page component={Leaderboard} currentPath="/leaderboard" />
          </ProtectedRoute>
        }
      />

      <Route
        path="/live"
        element={
          <ProtectedRoute allowGameSession>
            <Page component={LiveGame} currentPath="/live" />
          </ProtectedRoute>
        }
      />

      <Route
        path="/admin"
        element={
          <ProtectedRoute adminOnly>
            <Page component={AdminDashboard} currentPath="/admin" />
          </ProtectedRoute>
        }
      />

      <Route
        path="/admin/subscriptions"
        element={
          <ProtectedRoute adminOnly>
            <Page component={AdminSubscriptions} currentPath="/admin/subscriptions" />
          </ProtectedRoute>
        }
      />

      <Route
        path="/admin/schedule"
        element={
          <ProtectedRoute adminOnly>
            <Page component={AdminSchedule} currentPath="/admin/schedule" />
          </ProtectedRoute>
        }
      />

      <Route
        path="/admin/players"
        element={
          <ProtectedRoute adminOnly>
            <Page component={AdminPlayers} currentPath="/admin/players" />
          </ProtectedRoute>
        }
      />

      <Route
        path="/admin/bonuses"
        element={
          <ProtectedRoute adminOnly>
            <Page component={AdminBonuses} currentPath="/admin/bonuses" />
          </ProtectedRoute>
        }
      />

      <Route path="*" element={<Navigate to="/login" replace />} />
    </Routes>
  );
}

export default function App() {
  return (
    <AuthProvider>
      <AppRoutes />
    </AuthProvider>
  );
}
