#ifndef GLOBAL_H
#define GLOBAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <ArduinoJson.h>
#include "time.h"
#include <addons/TokenHelper.h>
#include <mbedtls/sha256.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>

// =================== LCD ===================
#define TFT_CS   5
#define TFT_DC   4
#define TFT_RST  2

extern Adafruit_ILI9341 tft;

void lcd_init();
void lcd_print_text(int x, int y, const String &text);
void lcd_clear();

// =================== Touch Screen ===================
#define TOUCH_CS 15

extern XPT2046_Touchscreen ts;

struct TouchPoint {
  bool pressed;
  int x;
  int y;
};

void touch_init();
TouchPoint getTouchPoint();
bool pointInRect(TouchPoint p, int x, int y, int w, int h);

// =================== DC Motors - XY mechanism ===================

#define X_IN1 26
#define X_IN2 25
#define X_ENABLE 33

#define Y_IN1 32
#define Y_IN2 21
#define Y_ENABLE 22

#define MOTOR_X_PWM_CHANNEL 4
#define MOTOR_Y_PWM_CHANNEL 5

#define MOTOR_PWM_FREQ 1000
#define MOTOR_PWM_RESOLUTION 8

#define DEFAULT_MOTOR_SPEED_X 240
#define DEFAULT_MOTOR_SPEED_Y 255



// Time-based calibration: how many milliseconds a motor must run to move one cm.
// After a physical test on the table this can be increased or decreased.
#define CM_TO_TIME_MS_X 44
#define CM_TO_TIME_MS_Y 44

void init_dc_motors();
void setMotorX(int direction, int speedValue);
void setMotorY(int direction, int speedValue);
void stopMotorX();
void stopMotorY();
void stopMotors();

// =================== Servo - puck hit ===================
#define SERVO_PIN 13
#define SERVO_REST_ANGLE 90
#define SERVO_HIT_ANGLE 130
#define SERVO_HIT_DELAY_MS 220
#define SERVO_RETURN_DELAY_MS 220

#define SERVO_CENTER_ADVANCE_CM 0.35f
#define SERVO_CENTER_ADVANCE_SPEED 145
#define SERVO_CENTER_ADVANCE_MIN_MS 35UL

extern Servo hitServo;

void init_servo();
void hitPuck();

// =================== IR goal sensors ===================
// Both IR modules are active LOW: no puck = HIGH, puck detected = LOW.
#define ROBOT_GOAL_IR_PIN 27
#define PLAYER_GOAL_IR_PIN 34

void init_ir_goal_sensors();
bool consumeRobotGoalIR();
bool consumePlayerGoalIR();
// =================== DistanceSensor ===================
#define DIST_TRIG_PIN 16
#define DIST_ECHO_PIN 35
#define DIST_ECHO_TIMEOUT_US 6000UL
#define DISTANCE_CHECK_INTERVAL_MS 100UL

void init_distance_Sensor();
float getDistanceCm();

// =================== LED Matrix ===================
#define LED_PIN 17
#define NUM_LEDS 256   // 16x16

extern Adafruit_NeoPixel strip;

// LED function declarations
void init_led();

void ledSetAllRed();
void ledSetAllGreen();
void ledSetAllBlue();
void ledSetAllWhite();
void ledOff();

void ledReadyEffect();
void ledGameActiveEffect();
void ledGoalEffect();
void ledViolationEffect();
void ledGameOverEffect();
void ledSoftBootLight();

uint32_t wheelColor(byte pos);


// =================== Speaker ===================
#define SPEAKER_PIN 14
#define SPEAKER_CHANNEL 6

void init_speaker();
void speakerBeep(int frequency, int durationMs);

void speakerStartGame();
void speakerGoal();
void speakerViolation();
void speakerGameOver();


// =================== ESP-NOW bracelet ===================
// The main ESP creates this local access point only to keep both ESPs
// on the same Wi-Fi radio channel. Internet access is not required.
#define BRACELET_AP_SSID "AIRHOCKEY_DIRECT"
#define BRACELET_AP_PASSWORD "12345678"

#define BRACELET_MESSAGE_VERSION 1
#define BRACELET_REQUIRED_TOUCH_MS 2000UL
#define BRACELET_PACKET_TIMEOUT_MS 350UL

struct BraceletMessage {
  uint8_t version;
  uint32_t sequence;
  bool touchRaw;
  bool braceletWorn;
  uint32_t continuousTouchMs;
  int movementState;
  float movementPower;
};

extern volatile bool braceletPacketReceived;
extern volatile bool braceletWorn;
extern volatile bool braceletTouchRaw;
extern volatile uint32_t braceletContinuousTouchMs;
extern volatile uint32_t braceletLastPacketMillis;
extern volatile int braceletMovementState;
extern volatile float braceletMovementPower;

void initEspNowBracelet();
bool isBraceletConnected();
bool isBraceletReadyForGame();
uint32_t getBraceletContinuousTouchMs();
void resetBraceletAuthorization();
void showWearBraceletScreen();
void handleWearBraceletScreen();
void pauseGameForBracelet();
void showBraceletRemovedScreen();
void handleBraceletPausedScreen();

// =================== WiFi / Time / Firebase ===================
extern bool networkAvailable;
void connect();
void init_time();
void init_firebase();
void get_document(const char* path);
void send_game_data(int table, int p1, int p2, int speed, int timeSec, const String& playerName = "Player");

// =================== Web Server ===================
extern WebServer server;
void init_react_server();
void handle_react_server();
void sendJson(int statusCode, const String& json);
void sendOptions();

void handleMoveCommand();
void handleRobotStatus();
void handlePuckUpdate();

// =================== Firebase globals ===================
extern FirebaseData fbdo;
extern FirebaseAuth auth;
extern FirebaseConfig config;

// =================== Callback ===================
void tokenStatusCallback(TokenInfo info);

// =================== Constants ===================
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define API_KEY "YOUR_FIREBASE_API_KEY"
#define FIREBASE_PROJECT_ID "YOUR_FIREBASE_PROJECT_ID"
#define DATABASE_ID "(default)"
#define USER_EMAIL "YOUR_FIREBASE_USER_EMAIL"
#define USER_PASSWORD "YOUR_FIREBASE_USER_PASSWORD"

// =================== Python Camera Server ===================

#define PYTHON_SERVER_IP "192.168.1.102"
#define PYTHON_SERVER_PORT 8001

bool startAutomaticCamera();
bool stopAutomaticCamera();

// =================== Puck tracking on main ESP32 ===================

struct PuckObservation {
  bool detected = false;
  bool stationary = false;
  uint32_t frameId = 0;
  uint32_t capturedAtMs = 0;
  float xPixel = 0.0f;
  float yPixel = 0.0f;
  int imageWidth = 0;
  int imageHeight = 0;
  float area = 0.0f;
  float radius = 0.0f;
  float confidence = 0.0f;
};

struct PuckTrackingConfig {
  bool robotOnLeftSide;
  bool flipCameraX;
  bool flipCameraY;

  float roiMinXRatio;
  float roiMaxXRatio;
  float roiMinYRatio;
  float roiMaxYRatio;

  float defenseLineXRatio;

  float velocityFilterAlpha;
  float accelerationFilterAlpha;

  float minimumApproachSpeedXPixelsPerSecond;
  float maximumValidPuckSpeedPixelsPerSecond;
  float minimumDetectionConfidence;

  float minimumDeltaTimeSeconds;
  float maximumDeltaTimeSeconds;

  float level1MaximumInterceptTimeSeconds;
  float level2MaximumInterceptTimeSeconds;
  float level3MaximumInterceptTimeSeconds;
  float level4MaximumInterceptTimeSeconds;

  float minimumTargetChangeCm;
  unsigned long minimumRobotCommandIntervalMs;
  unsigned long maximumTargetAgeMs;
  int lostFramesBeforeReset;

  bool useAccelerationInPrediction;
  float hitWindowSeconds;
};

struct PuckTrackingState {
  bool hasPosition = false;
  bool hasVelocity = false;

  float positionX = 0.0f;
  float positionY = 0.0f;
  float previousPositionX = 0.0f;
  float previousPositionY = 0.0f;

  float velocityX = 0.0f;
  float velocityY = 0.0f;
  float accelerationX = 0.0f;
  float accelerationY = 0.0f;
  float speed = 0.0f;

  uint32_t lastCaptureTimeMs = 0;
  uint32_t lastFrameId = 0;
  int imageWidth = 0;
  int imageHeight = 0;
  int lostFrameCount = 0;
};

struct PendingRobotTarget {
  bool valid = false;
  float xCm = 0.0f;
  float yCm = 0.0f;
  float interceptTimeSeconds = 0.0f;
  unsigned long generatedAtMillis = 0;
};

extern PuckTrackingConfig puckTrackingConfig;
extern PuckTrackingState puckTrackingState;
extern PendingRobotTarget pendingRobotTarget;
extern bool robotMovementBusy;

void init_puck_tracking();
void resetPuckTracking();
void updatePuckTracking(const PuckObservation &observation);
void processPuckTracking();
bool shouldLockCurrentMovementForServo();

// =================== Game Variables - RobotMove ===================
float currentX = 0;
float currentY = 0;

int tableNumber = 1;
int player1Score = 0;
int player2Score = 0;
int puckSpeed = 0;
int gameTimeSeconds = 0;
String playerName = "Player";

unsigned long lastFirebaseUpdate = 0;
unsigned long lastGoalMillis = 0;

#define GOAL_COOLDOWN_MS 1500

#define MIN_SAFE_DISTANCE_CM 4.0
#define ACCEL_MOVEMENT_THRESHOLD 13.0

#define MIN_X_CM 0.0f
#define MAX_X_CM 12.5f

#define MIN_Y_CM 0.0f
#define MAX_Y_CM 15.0f

#define START_X_CM 0.0f
#define START_Y_CM 0.0f

bool moveToPositionCm(float xCm, float yCm);
void moveToDefensePosition();
void moveToStartPosition();

// =========================
// Screen States - Touch menu
// =========================

enum ScreenState {
  SCREEN_ENTER_CODE,
  SCREEN_CHECKING_CODE,
  SCREEN_GAME_FOUND,
  SCREEN_SELECT_LEVEL,
  SCREEN_SELECT_MODE,
  SCREEN_READY_TO_START,
  SCREEN_WAIT_BRACELET,
  SCREEN_BRACELET_PAUSED,
  SCREEN_LIVE_GAME,
  SCREEN_ERROR
};

enum GameModeType {
  MODE_FIRST_TO_7,
  MODE_TIMER_3_MINUTES
};

ScreenState currentScreen = SCREEN_ENTER_CODE;

String enteredGameCode = "";
String selectedGameId = "";
String selectedGameCode = "";

int selectedLevel = 1;
GameModeType selectedGameMode = MODE_FIRST_TO_7;

String screenErrorMessage = "";
int currentDigit = 0;

int playerScore = 0;
int robotScore = 0;

unsigned long gameStartMillis = 0;
unsigned long braceletPauseStartedMillis = 0;

// =========================
// Game menu function prototypes
// =========================
void showEnterCodeScreen();
void handleEnterCodeScreen();
void showCheckingCodeScreen();
void checkGameCodeFromScreen();
void showGameFoundScreen();
void handleGameFoundScreen();
void showSelectLevelScreen();
void handleSelectLevelScreen();
void showSelectModeScreen();
void handleSelectModeScreen();
void showReadyToStartScreen();
void handleReadyToStartScreen();
void showLiveGameScreen();
void updateLiveGameScreen();
void finishLiveGame();
void showErrorScreen();
void handleErrorScreen();
void resetGameSelection();
bool startSelectedGame();
String selectedModeToString();
String getGameModeText();

// loaded from FuncationReact.ino
bool loadGameByCode(String code, String &foundDocId, FirebaseJson &foundDoc);

// loaded from FuncationReact.ino
void sendOptions();
void sendJson(int code, const String& json);

#endif