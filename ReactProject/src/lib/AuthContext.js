import React, { createContext, useContext, useMemo, useState } from "react";

const AuthContext = createContext();

export function AuthProvider({ children }) {
  const [currentUser, setCurrentUser] = useState(() => {
    const saved = localStorage.getItem("air_hockey_user");
    return saved ? JSON.parse(saved) : null;
  });

  const [activeGame, setActiveGame] = useState(() => {
    const saved = localStorage.getItem("air_hockey_active_game");
    return saved ? JSON.parse(saved) : null;
  });

  const login = (userData) => {
    localStorage.setItem("air_hockey_user", JSON.stringify(userData));
    setCurrentUser(userData);
  };

  const logout = () => {
    localStorage.removeItem("air_hockey_user");
    localStorage.removeItem("air_hockey_active_game");
    setCurrentUser(null);
    setActiveGame(null);
  };

  const setGameSession = (gameData) => {
    localStorage.setItem("air_hockey_active_game", JSON.stringify(gameData));
    setActiveGame(gameData);
  };

  const clearGameSession = () => {
    localStorage.removeItem("air_hockey_active_game");
    setActiveGame(null);
  };

  const value = useMemo(
    () => ({
      currentUser,
      activeGame,
      login,
      logout,
      setGameSession,
      clearGameSession,
    }),
    [currentUser, activeGame]
  );

  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>;
}

export function useAuth() {
  return useContext(AuthContext);
}
