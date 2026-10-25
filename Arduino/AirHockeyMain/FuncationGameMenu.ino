// =====================================================
// FuncationGameMenu.ino
// Touch screen menu - dark website style
// Enter code -> Check Firebase -> Select level
// -> Select mode -> Start game -> Live game
// =====================================================


// =====================================================
// Firestore helper functions
// =====================================================

String firestoreStringField(FirebaseJson &json, String fieldName, String defaultValue = "") {
  FirebaseJsonData data;

  String pathString = "fields/" + fieldName + "/stringValue";
  String pathInteger = "fields/" + fieldName + "/integerValue";
  String pathBoolean = "fields/" + fieldName + "/booleanValue";

  if (json.get(data, pathString) && data.success) {
    return data.stringValue;
  }

  if (json.get(data, pathInteger) && data.success) {
    return String(data.intValue);
  }

  if (json.get(data, pathBoolean) && data.success) {
    return data.boolValue ? "true" : "false";
  }

  return defaultValue;
}

int firestoreIntField(FirebaseJson &json, String fieldName, int defaultValue = 0) {
  FirebaseJsonData data;

  String pathInteger = "fields/" + fieldName + "/integerValue";
  String pathDouble = "fields/" + fieldName + "/doubleValue";
  String pathString = "fields/" + fieldName + "/stringValue";

  if (json.get(data, pathInteger) && data.success) {
    return data.intValue;
  }

  if (json.get(data, pathDouble) && data.success) {
    return data.intValue;
  }

  if (json.get(data, pathString) && data.success) {
    return data.stringValue.toInt();
  }

  return defaultValue;
}


// =====================================================
// UI colors - website style
// =====================================================

#define UI_BG          0x0841
#define UI_CARD        0x1082
#define UI_CARD_2      0x18C3
#define UI_DARK        0x0008

#define UI_CYAN        0x07FF
#define UI_CYAN_DARK   0x04B6
#define UI_BLUE        0x255F
#define UI_PURPLE      0x881F
#define UI_PURPLE_2    0xA81F

#define UI_GREEN       0x07E0
#define UI_RED         0xF800
#define UI_ORANGE      0xFD20
#define UI_WHITE       0xFFFF
#define UI_GRAY        0x8410
#define UI_LIGHT_GRAY  0xC618

#define UI_PANEL       UI_CARD
#define UI_PANEL_2     UI_CARD_2
#define UI_DARK        0x0008


// =====================================================
// UI helpers
// =====================================================

void clearGameScreen() {
  tft.fillScreen(UI_BG);
}

void drawRoundPanel(int x, int y, int w, int h) {
  tft.fillRoundRect(x, y, w, h, 12, UI_CARD);
  tft.drawRoundRect(x, y, w, h, 12, UI_CARD_2);
}

void drawHeader(String title, String subtitle = "") {
  clearGameScreen();

  drawRoundPanel(8, 6, 304, 58);

  tft.fillRoundRect(132, 14, 56, 28, 12, UI_DARK);
  tft.drawRoundRect(132, 14, 56, 28, 12, UI_CYAN);

  tft.setTextColor(UI_CYAN);
  tft.setTextSize(2);
  tft.setCursor(146, 20);
  tft.print("G");

  tft.setTextColor(UI_WHITE);
  tft.setTextSize(2);
  tft.setCursor(72, 43);
  tft.print(title);

  if (subtitle != "") {
    tft.setTextColor(UI_LIGHT_GRAY);
    tft.setTextSize(1);
    tft.setCursor(80, 60);
    tft.print(subtitle);
  }
}

void drawPageTitle(String title, String subtitle = "") {
  tft.setTextColor(UI_WHITE);
  tft.setTextSize(2);
  tft.setCursor(18, 12);
  tft.print(title);

  if (subtitle != "") {
    tft.setTextColor(UI_CYAN);
    tft.setTextSize(1);
    tft.setCursor(18, 34);
    tft.print(subtitle);
  }
}

void drawInputBox(int x, int y, int w, int h, String text) {
  tft.fillRoundRect(x, y, w, h, 8, UI_CARD_2);
  tft.drawRoundRect(x, y, w, h, 8, UI_GRAY);

  tft.setTextColor(UI_LIGHT_GRAY);
  tft.setTextSize(1);
  tft.setCursor(x + 10, y + 6);
  tft.print("Game code");

  tft.setTextColor(UI_WHITE);
  tft.setTextSize(2);
  tft.setCursor(x + 10, y + 24);
  tft.print(text);
}

void drawText1(int x, int y, String text, uint16_t color = UI_LIGHT_GRAY) {
  tft.setTextColor(color);
  tft.setTextSize(1);
  tft.setCursor(x, y);
  tft.print(text);
}

void drawText2(int x, int y, String text, uint16_t color = UI_WHITE) {
  tft.setTextColor(color);
  tft.setTextSize(2);
  tft.setCursor(x, y);
  tft.print(text);
}

void drawText3(int x, int y, String text, uint16_t color = UI_WHITE) {
  tft.setTextColor(color);
  tft.setTextSize(3);
  tft.setCursor(x, y);
  tft.print(text);
}

void drawGradientButton(int x, int y, int w, int h, String text) {
  // Simulates a gradient: half turquoise, half purple
  tft.fillRoundRect(x, y, w, h, 10, UI_CYAN_DARK);
  tft.fillRoundRect(x + (w / 2), y, w / 2, h, 10, UI_PURPLE_2);
  tft.drawRoundRect(x, y, w, h, 10, UI_CYAN);

  tft.setTextColor(UI_WHITE);
  tft.setTextSize(2);
  tft.setCursor(x + 14, y + (h / 2) - 8);
  tft.print(text);
}

void drawButton(int x, int y, int w, int h, String text, uint16_t color) {
  // Dark shadow
  tft.fillRoundRect(x + 3, y + 3, w, h, 10, 0x0000);

  // Button body
  tft.fillRoundRect(x, y, w, h, 10, color);

  // Light top strip — gives a sense of depth
  tft.fillRoundRect(x + 3, y + 3, w - 6, 7, 6, UI_WHITE);

  // Frame
  tft.drawRoundRect(x, y, w, h, 10, UI_WHITE);

  // Text
  tft.setTextColor(UI_WHITE);
  tft.setTextSize(2);

  int textX = x + 10;
  int textY = y + (h / 2) - 8;

  if (text.length() == 1) {
    textX = x + (w / 2) - 6;
  }

  if (text == "DEL") {
    textX = x + 10;
  }

  if (text == "OK") {
    textX = x + 18;
  }

  tft.setCursor(textX, textY);
  tft.print(text);
}


void drawKeyButton(int x, int y, int w, int h, String text, uint16_t borderColor, uint16_t fillColor) {
  // Shadow
  tft.fillRoundRect(x + 3, y + 3, w, h, 10, UI_DARK);

  // Dark inner background
  tft.fillRoundRect(x, y, w, h, 10, fillColor);

  // Colored frame
  tft.drawRoundRect(x, y, w, h, 5, borderColor);
  tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 9, borderColor);

  // Small glowing strip on top
  //tft.drawFastHLine(x + 10, y + 5, w - 20, borderColor);

  // Text
  tft.setTextColor(UI_WHITE);
  tft.setTextSize(2);

  int textX = x + (w / 2) - 6;
  int textY = y + (h / 2) - 8;

  if (text == "DEL") {
    textX = x + 8;
  }

  if (text == "OK") {
    textX = x + 16;
  }

  tft.setCursor(textX, textY);
  tft.print(text);
}


void drawOutlineButton(int x, int y, int w, int h, String text, uint16_t borderColor) {
  tft.fillRoundRect(x, y, w, h, 10, UI_CARD_2);
  tft.drawRoundRect(x, y, w, h, 10, borderColor);

  tft.setTextColor(borderColor);
  tft.setTextSize(2);
  tft.setCursor(x + 10, y + (h / 2) - 8);
  tft.print(text);
}

bool touchButton(int x, int y, int w, int h) {
  TouchPoint p = getTouchPoint();

  if (!p.pressed) {
    return false;
  }

  Serial.print("Button rect: ");
  Serial.print(x);
  Serial.print(",");
  Serial.print(y);
  Serial.print(",");
  Serial.print(w);
  Serial.print(",");
  Serial.print(h);
  Serial.print(" point=");
  Serial.print(p.x);
  Serial.print(",");
  Serial.println(p.y);

  if (pointInRect(p, x, y, w, h)) {
    delay(250);
    return true;
  }

  return false;
}

String selectedModeToString() {
  if (selectedGameMode == MODE_FIRST_TO_7) {
    return "first_to_7";
  }

  return "timer_3_minutes";
}

String getGameModeText() {
  if (selectedGameMode == MODE_FIRST_TO_7) {
    return "First to 7";
  }

  return "Timer 3 min";
}

void resetGameSelection() {
  enteredGameCode = "";
  selectedGameId = "";
  selectedGameCode = "";
  selectedLevel = 1;
  selectedGameMode = MODE_FIRST_TO_7;
  currentDigit = 0;
  screenErrorMessage = "";

  playerScore = 0;
  robotScore = 0;
  player1Score = 0;
  player2Score = 0;
}


// =====================================================
// Screen 1: Enter game code
// =====================================================

void showEnterCodeScreen() {
  currentScreen = SCREEN_ENTER_CODE;
  clearGameScreen();

  // Main card like on the website
  tft.fillRoundRect(8, 8, 304, 224, 14, UI_CARD);
  tft.drawRoundRect(8, 8, 304, 224, 14, UI_CARD_2);

  // Title — without the G square
  drawText2(66, 22, "AirHockey Robot", UI_WHITE);
  drawText1(72, 46, "Enter scheduled game code", UI_LIGHT_GRAY);

  // Code box
  tft.fillRoundRect(22, 70, 276, 34, 8, UI_PANEL);
  tft.drawRoundRect(22, 70, 276, 34, 8, UI_CYAN);

  tft.setTextColor(UI_GRAY);
  tft.setTextSize(1);
  tft.setCursor(32, 78);
  tft.print("Code");

  tft.setTextColor(UI_WHITE);
  tft.setTextSize(2);
  tft.setCursor(80, 82);
  tft.print(enteredGameCode);

  // =========================
  // Keypad — same relative position, moved up a little
  // =========================

  // Row 1
  drawKeyButton(25, 116, 58, 28, "1", UI_CYAN, UI_PANEL_2);
  drawKeyButton(95, 116, 58, 28, "2", UI_CYAN, UI_PANEL_2);
  drawKeyButton(165, 116, 58, 28, "3", UI_CYAN, UI_PANEL_2);
  drawKeyButton(235, 116, 58, 28, "0", UI_PURPLE, UI_PANEL_2);

  // Row 2
  drawKeyButton(25, 151, 58, 28, "4", UI_CYAN, UI_PANEL_2);
  drawKeyButton(95, 151, 58, 28, "5", UI_CYAN, UI_PANEL_2);
  drawKeyButton(165, 151, 58, 28, "6", UI_CYAN, UI_PANEL_2);
  drawKeyButton(235, 151, 58, 28, "DEL", UI_ORANGE, UI_PANEL_2);

  // Row 3
  drawKeyButton(25, 186, 58, 28, "7", UI_CYAN, UI_PANEL_2);
  drawKeyButton(95, 186, 58, 28, "8", UI_CYAN, UI_PANEL_2);
  drawKeyButton(165, 186, 58, 28, "9", UI_CYAN, UI_PANEL_2);
  drawKeyButton(235, 186, 58, 28, "OK", UI_GREEN, UI_PANEL_2);
}

// This function uses only one point per press.
// It was the most stable option in testing.
void handleEnterCodeScreen() {
  TouchPoint p = getTouchPoint();

  if (!p.pressed) {
    return;
  }

  Serial.print("FINAL TOUCH POINT: ");
  Serial.print(p.x);
  Serial.print(",");
  Serial.println(p.y);

  String key = "";


  // Row 1
  if (p.y >= 110 && p.y < 150) {
    if (p.x < 100) {
      key = "1";
    } else if (p.x < 170) {
      key = "2";
    } else if (p.x < 232) {
      key = "3";
    } else {
      key = "0";
    }
  }

  // Row 2
  else if (p.y >= 150 && p.y < 180) {
    if (p.x < 100) {
      key = "4";
    } else if (p.x < 170) {
      key = "5";
    } else if (p.x < 232) {
      key = "6";
    } else {
      key = "DEL";
    }
  }

  // Row 3
  else if (p.y >= 183 && p.y <= 239) {
    if (p.x < 100) {
      key = "7";
    } else if (p.x < 170) {
      key = "8";
    } else if (p.x < 232) {
      key = "9";
    } else {
      key = "OK";
    }
  }

  Serial.print("DETECTED KEY: ");
  Serial.println(key);

  if (key == "") {
    delay(250);
    return;
  }

  if (key == "DEL") {
    deleteLastDigit();
    delay(250);
    return;
  }

  if (key == "OK") {
    if (enteredGameCode.length() < 4) {
      screenErrorMessage = "Code too short";
      currentScreen = SCREEN_ERROR;
      showErrorScreen();
      delay(250);
      return;
    }

    currentScreen = SCREEN_CHECKING_CODE;
    showCheckingCodeScreen();
    checkGameCodeFromScreen();
    delay(250);
    return;
  }

  addDigitToCode(key);
  delay(250);
}

void addDigitToCode(String digit) {
  if (enteredGameCode.length() < 10) {
    enteredGameCode += digit;
    showEnterCodeScreen();
  }
}

void deleteLastDigit() {
  if (enteredGameCode.length() > 0) {
    enteredGameCode.remove(enteredGameCode.length() - 1);
  }

  showEnterCodeScreen();
}


// =====================================================
// Screen 2: Checking code
// =====================================================

void showCheckingCodeScreen() {
  currentScreen = SCREEN_CHECKING_CODE;
  clearGameScreen();

  drawRoundPanel(10, 18, 300, 190);
  drawText2(78, 60, "Checking...", UI_CYAN);
  drawText1(76, 92, "Searching in Firebase", UI_LIGHT_GRAY);
  drawInputBox(35, 122, 250, 50, enteredGameCode);
}

void checkGameCodeFromScreen() {
  String foundDocId = "";
  FirebaseJson foundDoc;

  Serial.println("========== CHECK GAME CODE ==========");
  Serial.print("Entered code: ");
  Serial.println(enteredGameCode);

  bool found = loadGameByCode(enteredGameCode, foundDocId, foundDoc);

  Serial.print("Found: ");
  Serial.println(found ? "YES" : "NO");
  Serial.print("Found doc id: ");
  Serial.println(foundDocId);

  if (!found) {
    screenErrorMessage = "Game not found";
    currentScreen = SCREEN_ERROR;
    showErrorScreen();
    return;
  }

  String status = firestoreStringField(foundDoc, "status", "scheduled");
  status.trim();
  status.toLowerCase();

  Serial.print("Game status: ");
  Serial.println(status);

  if (status != "scheduled") {
    screenErrorMessage = "Game already used";
    currentScreen = SCREEN_ERROR;
    showErrorScreen();
    return;
  }

  selectedGameId = foundDocId;
  selectedGameCode = firestoreStringField(foundDoc, "game_code", enteredGameCode);

  if (selectedGameCode == "") {
    selectedGameCode = enteredGameCode;
  }

  currentScreen = SCREEN_GAME_FOUND;
  showGameFoundScreen();
}


// =====================================================
// Screen 3: Game found
// =====================================================

void showGameFoundScreen() {
  clearGameScreen();

  drawRoundPanel(10, 15, 300, 205);
  drawText2(72, 45, "Game Found", UI_WHITE);
  drawText1(72, 73, "Scheduled game verified", UI_CYAN);

  drawInputBox(35, 100, 250, 48, selectedGameCode);

  drawGradientButton(35, 168, 250, 42, "Continue");
}

void handleGameFoundScreen() {
  if (touchButton(35, 168, 250, 42)) {
    currentScreen = SCREEN_SELECT_LEVEL;
    showSelectLevelScreen();
    return;
  }
}


// =====================================================
// Screen 4: Select level
// =====================================================

void showSelectLevelScreen() {
  clearGameScreen();

  drawRoundPanel(10, 15, 300, 205);
  drawText2(70, 42, "Select Level", UI_WHITE);
  drawText1(65, 68, "Choose robot difficulty", UI_LIGHT_GRAY);

  drawOutlineButton(30, 90, 120, 42, "Level 1", UI_CYAN);
  drawOutlineButton(170, 90, 120, 42, "Level 2", UI_CYAN);

  drawOutlineButton(30, 145, 120, 42, "Level 3", UI_PURPLE);
  drawOutlineButton(170, 145, 120, 42, "Level 4", UI_PURPLE);

  drawButton(105, 198, 110, 28, "Back", UI_RED);
}

void selectLevel(int level) {
  selectedLevel = level;

  clearGameScreen();
  drawRoundPanel(35, 60, 250, 110);
  drawText2(65, 90, "Level Selected", UI_WHITE);
  drawText2(105, 125, "Level " + String(level), UI_CYAN);

  delay(700);

  currentScreen = SCREEN_SELECT_MODE;
  showSelectModeScreen();
}

void handleSelectLevelScreen() {
  if (touchButton(30, 90, 120, 42)) {
    selectLevel(1);
    return;
  }

  if (touchButton(170, 90, 120, 42)) {
    selectLevel(2);
    return;
  }

  if (touchButton(30, 145, 120, 42)) {
    selectLevel(3);
    return;
  }

  if (touchButton(170, 145, 120, 42)) {
    selectLevel(4);
    return;
  }

  if (touchButton(105, 198, 110, 28)) {
    currentScreen = SCREEN_GAME_FOUND;
    showGameFoundScreen();
    return;
  }
}


// =====================================================
// Screen 5: Select game mode
// =====================================================

void showSelectModeScreen() {
  clearGameScreen();

  drawRoundPanel(10, 15, 300, 205);
  drawText2(78, 42, "Game Mode", UI_WHITE);
  drawText1(78, 68, "Select win condition", UI_LIGHT_GRAY);

  drawOutlineButton(35, 95, 250, 42, "First to 7", UI_CYAN);
  drawOutlineButton(35, 150, 250, 42, "Timer 3m", UI_PURPLE);

  drawButton(105, 200, 110, 28, "Back", UI_RED);
}

void handleSelectModeScreen() {
  if (touchButton(35, 95, 250, 42)) {
    selectedGameMode = MODE_FIRST_TO_7;
    currentScreen = SCREEN_READY_TO_START;
    showReadyToStartScreen();
    return;
  }

  if (touchButton(35, 150, 250, 42)) {
    selectedGameMode = MODE_TIMER_3_MINUTES;
    currentScreen = SCREEN_READY_TO_START;
    showReadyToStartScreen();
    return;
  }

  if (touchButton(105, 200, 110, 28)) {
    currentScreen = SCREEN_SELECT_LEVEL;
    showSelectLevelScreen();
    return;
  }
}


// =====================================================
// Screen 6: Ready to start
// =====================================================

void showReadyToStartScreen() {
  clearGameScreen();

  drawRoundPanel(10, 15, 300, 205);
  drawText2(52, 38, "Ready To Start", UI_WHITE);
  drawText1(76, 64, "Confirm game settings", UI_LIGHT_GRAY);

  drawText1(35, 92, "Code", UI_CYAN);
  drawText2(35, 108, selectedGameCode, UI_WHITE);

  drawText1(35, 138, "Level", UI_CYAN);
  drawText2(35, 154, String(selectedLevel), UI_WHITE);

  drawText1(145, 138, "Mode", UI_CYAN);
  drawText2(145, 154, getGameModeText(), UI_WHITE);

  drawGradientButton(35, 190, 250, 38, "Start Game");
}

void handleReadyToStartScreen() {
  if (!touchButton(
        35,
        190,
        250,
        38
      )) {
    return;
  }

  /*
    Do not start Firebase/game timers yet.
    First require the bracelet to be worn continuously.
  */
  resetBraceletAuthorization();
  currentScreen =
    SCREEN_WAIT_BRACELET;

  showWearBraceletScreen();
}


// =====================================================
// Bracelet authorization screen
// =====================================================

void showWearBraceletScreen() {
  stopMotors();
  ledSetAllRed();

  clearGameScreen();

  drawRoundPanel(
    10,
    15,
    300,
    205
  );

  drawText2(
    54,
    38,
    "Wear Bracelet",
    UI_WHITE
  );

  drawText1(
    57,
    72,
    "Put the bracelet on the wrist",
    UI_CYAN
  );

  drawText1(
    55,
    98,
    "Keep continuous contact",
    UI_LIGHT_GRAY
  );

  drawText2(
    92,
    128,
    "Waiting...",
    UI_RED
  );

  drawText1(
    56,
    172,
    "Game starts after 2 seconds",
    UI_LIGHT_GRAY
  );
}


void handleWearBraceletScreen() {
  static unsigned long lastUiUpdate =
    0;

  if (
    millis() - lastUiUpdate >=
    250UL
  ) {
    lastUiUpdate =
      millis();

    bool connected =
      isBraceletConnected();

    uint32_t heldMs =
      getBraceletContinuousTouchMs();

    tft.fillRect(
      50,
      122,
      230,
      50,
      UI_CARD
    );

    if (!connected) {
      drawText2(
        70,
        128,
        "Bracelet Offline",
        UI_RED
      );
    }
    else if (!braceletTouchRaw) {
      drawText2(
        92,
        128,
        "Waiting...",
        UI_ORANGE
      );
    }
    else {
      unsigned long remainingMs =
        heldMs >=
          BRACELET_REQUIRED_TOUCH_MS
          ? 0
          : BRACELET_REQUIRED_TOUCH_MS -
            heldMs;

      drawText2(
        74,
        128,
        "Hold " +
          String(
            (
              remainingMs +
              999UL
            ) /
            1000UL
          ) +
          " sec",
        UI_GREEN
      );
    }
  }

  if (!isBraceletReadyForGame()) {
    stopMotors();
    ledSetAllRed();
    return;
  }

  /*
    A valid continuous touch was received through ESP-NOW.
  */
  ledSetAllGreen();

  speakerBeep(
    950,
    110
  );

  delay(60);

  speakerBeep(
    1350,
    180
  );

  bool started =
    startSelectedGame();

  if (!started) {
    /*
      The bracelet authorization itself works without Internet.
      The current game menu still needs Firebase for a scheduled game.
    */
    ledSetAllRed();
    screenErrorMessage =
      networkAvailable
        ? "Start failed"
        : "No network for game data";

    currentScreen =
      SCREEN_ERROR;

    showErrorScreen();
    return;
  }

  speakerStartGame();

  currentScreen =
    SCREEN_LIVE_GAME;

  resetPuckTracking();
  showLiveGameScreen();

  Serial.println(
    "Bracelet approved - game started"
  );
}



// =====================================================
// Bracelet removed during an active game
// =====================================================

void pauseGameForBracelet() {
  if (
    currentScreen ==
    SCREEN_BRACELET_PAUSED
  ) {
    return;
  }

  stopMotors();
  resetPuckTracking();

  braceletPauseStartedMillis =
    millis();

  currentScreen =
    SCREEN_BRACELET_PAUSED;

  ledSetAllRed();

  speakerBeep(
    420,
    180
  );

  showBraceletRemovedScreen();

  Serial.println(
    "Bracelet removed - game paused and motors stopped"
  );
}


void showBraceletRemovedScreen() {
  stopMotors();
  ledSetAllRed();

  clearGameScreen();

  drawRoundPanel(
    10,
    15,
    300,
    205
  );

  drawText2(
    43,
    36,
    "GAME PAUSED",
    UI_RED
  );

  drawText1(
    62,
    72,
    "Bracelet was removed",
    UI_WHITE
  );

  drawText1(
    47,
    96,
    "Put it back on your wrist",
    UI_CYAN
  );

  drawText2(
    92,
    128,
    "Waiting...",
    UI_ORANGE
  );

  drawText1(
    54,
    174,
    "Hold contact for 2 seconds",
    UI_LIGHT_GRAY
  );
}


void handleBraceletPausedScreen() {
  static unsigned long lastPauseUiUpdate =
    0;

  /*
    Motors and matrix remain in the safe state for every loop pass.
  */
  stopMotors();
  ledSetAllRed();

  if (
    millis() - lastPauseUiUpdate >=
    200UL
  ) {
    lastPauseUiUpdate =
      millis();

    bool connected =
      isBraceletConnected();

    uint32_t heldMs =
      getBraceletContinuousTouchMs();

    tft.fillRect(
      45,
      120,
      240,
      52,
      UI_CARD
    );

    if (!connected) {
      drawText2(
        68,
        128,
        "Bracelet Offline",
        UI_RED
      );
    }
    else if (!braceletTouchRaw) {
      drawText2(
        80,
        128,
        "Put It Back",
        UI_ORANGE
      );
    }
    else {
      unsigned long remainingMs =
        heldMs >=
          BRACELET_REQUIRED_TOUCH_MS
          ? 0UL
          : BRACELET_REQUIRED_TOUCH_MS -
            heldMs;

      drawText2(
        76,
        128,
        "Hold " +
          String(
            (
              remainingMs +
              999UL
            ) /
            1000UL
          ) +
          " sec",
        UI_GREEN
      );
    }
  }

  if (!isBraceletReadyForGame()) {
    return;
  }

  unsigned long pauseDuration =
    millis() -
    braceletPauseStartedMillis;

  /*
    Exclude the paused period from the three-minute game timer.
  */
  gameStartMillis +=
    pauseDuration;

  braceletPauseStartedMillis =
    0;

  ledSetAllGreen();

  speakerBeep(
    950,
    100
  );

  delay(50);

  speakerBeep(
    1350,
    150
  );

  resetPuckTracking();

  currentScreen =
    SCREEN_LIVE_GAME;

  showLiveGameScreen();

  Serial.println(
    "Bracelet restored - game resumed"
  );
}


// =====================================================
// Start game - update Firebase
// =====================================================

bool startSelectedGame() {
  if (selectedGameId == "") {
    return false;
  }

  String documentPath = "games/" + selectedGameId;

  FirebaseJson existingGame;

  if (!Firebase.Firestore.getDocument(
        &fbdo,
        FIREBASE_PROJECT_ID,
        DATABASE_ID,
        documentPath.c_str()
      )) {
    return false;
  }

  existingGame.setJsonData(fbdo.payload());

  String currentStatus = firestoreStringField(existingGame, "status", "scheduled");
  currentStatus.trim();
  currentStatus.toLowerCase();

  if (currentStatus != "scheduled") {
    return false;
  }

  FirebaseJson content;
  content.set("fields/status/stringValue", "in_progress");
  content.set("fields/skill_level/integerValue", selectedLevel);
  content.set("fields/game_mode/stringValue", selectedModeToString());
  content.set("fields/game_code/stringValue", selectedGameCode);
  content.set("fields/player1_score/integerValue", 0);
  content.set("fields/player2_score/integerValue", 0);
  content.set("fields/winner/stringValue", "");

  bool ok = Firebase.Firestore.patchDocument(
    &fbdo,
    FIREBASE_PROJECT_ID,
    DATABASE_ID,
    documentPath.c_str(),
    content.raw(),
    "status,skill_level,game_mode,game_code,player1_score,player2_score,winner"
  );

  if (ok) {
    playerScore = 0;
    robotScore = 0;
    player1Score = 0;
    player2Score = 0;

    puckSpeed = 0;
    gameTimeSeconds = 0;

    gameStartMillis = millis();
    lastFirebaseUpdate = 0;
    lastGoalMillis = 0;
  }

  return ok;
}

// =====================================================
// Screen 7: Live game
// =====================================================

void showLiveGameScreen() {
  clearGameScreen();

  drawRoundPanel(10, 15, 300, 205);
  drawText2(72, 38, "Game Running", UI_WHITE);
  drawText1(100, 64, getGameModeText(), UI_CYAN);

  drawText2(45, 95, "Player", UI_CYAN);
  drawText3(70, 125, String(player1Score), UI_WHITE);

  drawText2(185, 95, "Robot", UI_PURPLE);
  drawText3(210, 125, String(player2Score), UI_WHITE);

  drawText1(45, 185, "Level: " + String(selectedLevel), UI_LIGHT_GRAY);
  drawText1(45, 202, "Code: " + selectedGameCode, UI_LIGHT_GRAY);
}


void updateLiveGameScreen() {
  static unsigned long lastScreenUpdate = 0;

  if (millis() - lastScreenUpdate < 1000) {
    return;
  }

  lastScreenUpdate = millis();

  int elapsedSeconds = (millis() - gameStartMillis) / 1000;
  int remainingSeconds = 180 - elapsedSeconds;

  if (remainingSeconds < 0) {
    remainingSeconds = 0;
  }

  clearGameScreen();

  drawRoundPanel(10, 15, 300, 205);

  drawText2(72, 38, "Game Running", UI_WHITE);
  drawText1(100, 64, getGameModeText(), UI_CYAN);

  drawText2(45, 95, "Player", UI_CYAN);
  drawText3(70, 125, String(player1Score), UI_WHITE);

  drawText2(185, 95, "Robot", UI_PURPLE);
  drawText3(210, 125, String(player2Score), UI_WHITE);

  drawText1(45, 180, "Level: " + String(selectedLevel), UI_LIGHT_GRAY);

  if (selectedGameMode == MODE_TIMER_3_MINUTES) {
    drawText2(45, 200, "Time: " + String(remainingSeconds), UI_GREEN);
  } else {
    drawText2(45, 200, "Goal: 7", UI_GREEN);
  }
}

// =====================================================
// Finish game
// =====================================================

void finishLiveGame() {
  // Stop the motors first.
  stopMotors();

  // The Python loop stays active in the background.
  // Motion calculations on the ESP run only on the game screen.
  resetPuckTracking();
  Serial.println(
    "Game ended; Python camera loop remains active"
  );

  ledGameOverEffect();
  speakerGameOver();

  String winner = "";

  if (player1Score > player2Score) {
    winner = "Player";
  } else if (player2Score > player1Score) {
    winner = "Robot";
  } else {
    winner = "Draw";
  }

  if (selectedGameId != "") {
    String documentPath = "games/" + selectedGameId;

    FirebaseJson content;

    content.set(
      "fields/status/stringValue",
      "completed"
    );

    content.set(
      "fields/player1_score/integerValue",
      player1Score
    );

    content.set(
      "fields/player2_score/integerValue",
      player2Score
    );

    content.set(
      "fields/winner/stringValue",
      winner
    );

    content.set(
      "fields/is_active/booleanValue",
      false
    );

    content.set(
      "fields/game_time_seconds/integerValue",
      gameTimeSeconds
    );

    Firebase.Firestore.patchDocument(
      &fbdo,
      FIREBASE_PROJECT_ID,
      DATABASE_ID,
      documentPath.c_str(),
      content.raw(),
      "status,player1_score,player2_score,winner,"
      "is_active,game_time_seconds"
    );
  }

  clearGameScreen();

  drawRoundPanel(10, 20, 300, 195);

  drawText2(
    92,
    48,
    "Game Over",
    UI_WHITE
  );

  drawText2(
    45,
    95,
    "Winner: " + winner,
    UI_GREEN
  );

  drawText2(
    45,
    130,
    "P: " +
    String(player1Score) +
    " R: " +
    String(player2Score),
    UI_WHITE
  );

  drawGradientButton(
    45,
    170,
    230,
    38,
    "New Game"
  );

  currentScreen = SCREEN_ERROR;
  screenErrorMessage = "Game completed";
}

// =====================================================
// Error screen
// =====================================================

void showErrorScreen() {
  clearGameScreen();

  drawRoundPanel(10, 30, 300, 175);

  drawText2(122, 58, "Error", UI_RED);
  drawText2(35, 100, screenErrorMessage, UI_WHITE);

  drawGradientButton(45, 150, 230, 40, "Try Again");
}

void handleErrorScreen() {
  if (touchButton(45, 150, 230, 40)) {
    resetGameSelection();
    showEnterCodeScreen();
    return;
  }
}