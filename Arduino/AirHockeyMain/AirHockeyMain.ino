#include "Global.h"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS);

WebServer server(80);

Servo hitServo;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("========== ESP32 START ==========");
  Serial.println("BOOT START");

  lcd_init();
  lcd_clear();
  lcd_print_text(20, 20, "Booting...");
  Serial.println("LCD initialized");

  touch_init();
  Serial.println("Touch screen initialized");

  connect();
  Serial.println("WiFi setup completed");

  initEspNowBracelet();
  Serial.println("ESP-NOW bracelet initialized");

  init_time();
  Serial.println("Time initialized");

  init_firebase();
  Serial.println("Firebase initialized");

  init_puck_tracking();
  Serial.println("Puck tracking initialized");

  init_react_server();
  Serial.println("React server initialized");

  init_dc_motors();
  Serial.println("DC motors initialized");
  

  init_servo();
  Serial.println("Servo initialized");

  init_led();
  Serial.println("LED initialized");
  
  ledSoftBootLight();

  init_speaker();
  Serial.println("Speaker initialized");

  init_ir_goal_sensors();
  Serial.println("IR goal sensors initialized");

  init_distance_Sensor();
  Serial.println("Distance sensor initialized");

  lcd_clear();
  lcd_print_text(20, 20, "ESP32 Ready");
  Serial.println("ESP32 Ready");

  delay(1000);

  showEnterCodeScreen();
  Serial.println("Enter code screen shown");
}

void loop() {
  handle_react_server();

  switch (currentScreen) {
    case SCREEN_ENTER_CODE:
      handleEnterCodeScreen();
      break;

    case SCREEN_CHECKING_CODE:
      break;

    case SCREEN_GAME_FOUND:
      handleGameFoundScreen();
      break;

    case SCREEN_SELECT_LEVEL:
      handleSelectLevelScreen();
      break;

    case SCREEN_SELECT_MODE:
      handleSelectModeScreen();
      break;

    case SCREEN_READY_TO_START:
      handleReadyToStartScreen();
      break;

    case SCREEN_WAIT_BRACELET:
      handleWearBraceletScreen();
      break;

    case SCREEN_BRACELET_PAUSED:
      handleBraceletPausedScreen();
      break;

    case SCREEN_LIVE_GAME:
      runLiveGameLogic();
      break;

    case SCREEN_ERROR:
      handleErrorScreen();
      break;

    default:
      break;
  }
}

void runLiveGameLogic() {
  /*
    Highest safety priority: the bracelet must remain on the wrist
    during the entire game. Loss of touch or ESP-NOW packets pauses
    the game immediately and stops both motors.
  */
  if (!isBraceletReadyForGame()) {
    pauseGameForBracelet();
    return;
  }

  /*
    Execute camera targets only while the bracelet is confirmed.
  */
  processPuckTracking();

  updateLiveGameScreen();
  ledGameActiveEffect();

  /*
    Do not call pulseIn() on every loop iteration.
    A failed ultrasonic reading can otherwise delay camera handling.
  */
  static unsigned long lastDistanceCheckMillis = 0;
  static float lastDistanceCm = -1.0f;

  if (
    millis() -
      lastDistanceCheckMillis >=
    DISTANCE_CHECK_INTERVAL_MS
  ) {
    lastDistanceCheckMillis =
      millis();

    lastDistanceCm =
      getDistanceCm();
  }

  if (
    lastDistanceCm > 0.0f &&
    lastDistanceCm <
      MIN_SAFE_DISTANCE_CM
  ) {
    stopMotors();

    lcd_clear();
    lcd_print_text(
      20,
      40,
      "Safety Stop"
    );

    lcd_print_text(
      20,
      80,
      "Too close!"
    );

    ledViolationEffect();
    speakerViolation();

    delay(700);
    showLiveGameScreen();

    return;
  }

  if (
    millis() -
      lastGoalMillis >
    GOAL_COOLDOWN_MS
  ) {
    if (consumeRobotGoalIR()) {
      lastGoalMillis =
        millis();

      player1Score++;

      gameTimeSeconds =
        (
          millis() -
          gameStartMillis
        ) /
        1000;

      lcd_clear();
      lcd_print_text(
        20,
        40,
        "Goal!"
      );

      lcd_print_text(
        20,
        80,
        "Player +1"
      );

      lcd_print_text(
        20,
        120,
        "Score:"
      );

      lcd_print_text(
        20,
        160,
        String(player1Score) +
        " - " +
        String(player2Score)
      );

      ledGoalEffect();
      speakerGoal();

      delay(900);

      if (
        selectedGameMode ==
          MODE_FIRST_TO_7 &&
        player1Score >= 7
      ) {
        finishLiveGame();
        return;
      }

      showLiveGameScreen();
      return;
    }

    if (consumePlayerGoalIR()) {
      lastGoalMillis =
        millis();

      player2Score++;

      gameTimeSeconds =
        (
          millis() -
          gameStartMillis
        ) /
        1000;

      lcd_clear();
      lcd_print_text(
        20,
        40,
        "Goal!"
      );

      lcd_print_text(
        20,
        80,
        "Robot +1"
      );

      lcd_print_text(
        20,
        120,
        "Score:"
      );

      lcd_print_text(
        20,
        160,
        String(player1Score) +
        " - " +
        String(player2Score)
      );

      ledGoalEffect();
      speakerGoal();

      delay(900);

      if (
        selectedGameMode ==
          MODE_FIRST_TO_7 &&
        player2Score >= 7
      ) {
        finishLiveGame();
        return;
      }

      showLiveGameScreen();
      return;
    }
  }

  gameTimeSeconds =
    (
      millis() -
      gameStartMillis
    ) /
    1000;

  if (
    selectedGameMode ==
      MODE_TIMER_3_MINUTES &&
    gameTimeSeconds >= 180
  ) {
    finishLiveGame();
    return;
  }

  if (
    selectedGameMode ==
      MODE_FIRST_TO_7 &&
    (
      player1Score >= 7 ||
      player2Score >= 7
    )
  ) {
    finishLiveGame();
    return;
  }

  yield();
}
