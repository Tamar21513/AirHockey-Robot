#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// =====================================================
// Pins
// =====================================================

#define ACCEL_SDA_PIN 21
#define ACCEL_SCL_PIN 22


#define BRACELET_TOUCH_PIN 27

// =====================================================
// Main ESP local network
// =====================================================

#define MAIN_AP_SSID "AIRHOCKEY_DIRECT"
#define MAIN_AP_PASSWORD "12345678"

// ESP-NOW Broadcast
uint8_t broadcastAddress[] = {
  0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF
};

// =====================================================
// Bracelet settings
// =====================================================

#define MESSAGE_VERSION 1

/*
  Continuous touch for two seconds is required
  to confirm that the bracelet is being worn.
*/
#define REQUIRED_TOUCH_MS 2000UL

/*
  Only if there is no touch for 500ms in a row,
  the bracelet is considered removed.

  This prevents stopping because of a single unstable reading.
*/
#define TOUCH_LOST_CONFIRM_MS 500UL

/*
  Based on the measurements:

  No touch: 84–87
  Touch: 0–2

  Value less than or equal to 30 = touch.
  Value greater than or equal to 60 = no touch.
*/
#define TOUCH_ON_THRESHOLD 30
#define TOUCH_OFF_THRESHOLD 60

/*
  Number of readings averaged in each check.
*/
#define TOUCH_SAMPLE_COUNT 8

// Send the bracelet state five times per second
#define SEND_INTERVAL_MS 200UL

// Wait time for the ESP-NOW callback
#define SEND_CALLBACK_TIMEOUT_MS 700UL

// Network reconnect attempt
#define AP_RETRY_INTERVAL_MS 2000UL

// =====================================================
// Accelerometer settings
// =====================================================

#define MOVEMENT_THRESHOLD 4.0f
#define HIT_THRESHOLD 9.0f

Adafruit_ADXL345_Unified accelerometer =
  Adafruit_ADXL345_Unified(12345);

// =====================================================
// Message structure
//
// Must be identical to the struct on the main ESP.
// =====================================================

struct BraceletMessage {
  uint8_t version;
  uint32_t sequence;

  bool touchRaw;
  bool braceletWorn;
  uint32_t continuousTouchMs;

  int movementState;
  float movementPower;
};

BraceletMessage braceletMessage = {};

// =====================================================
// Global variables
// =====================================================

float accelerationBaseX = 0.0f;
float accelerationBaseY = 0.0f;
float accelerationBaseZ = 0.0f;

float currentMovementPower = 0.0f;

uint32_t messageSequence = 0;

// Current touch value
uint16_t currentTouchValue = 0;

// Touch state
bool touchDetectedState = false;
bool braceletWornState = false;

// Touch timing
unsigned long touchStartedMillis = 0;
unsigned long touchLostStartedMillis = 0;

// Communication timing
unsigned long lastSendMillis = 0;
unsigned long sendStartedMillis = 0;
unsigned long lastApAttemptMillis = 0;

bool espNowReady = false;

volatile bool sendInProgress = false;
volatile bool sendCallbackReceived = false;
volatile bool lastSendSucceeded = false;

// =====================================================
// ESP-NOW callback
// =====================================================

void onEspNowDataSent(
  const uint8_t *macAddress,
  esp_now_send_status_t status
) {
  lastSendSucceeded =
    status == ESP_NOW_SEND_SUCCESS;

  sendCallbackReceived = true;
  sendInProgress = false;
}

// =====================================================
// Direct ESP32 touch sensor
// =====================================================

void initializeTouchSensor() {
  /*
    No pinMode is needed for touchRead.

    GPIO 27 is connected directly to the aluminum foil.
    Do not connect VCC or GND to the foil.
  */

  Serial.println();
  Serial.println(
    "Direct ESP32 touch sensor initialized"
  );

  Serial.print(
    "Initial touch value: "
  );

  Serial.println(
    touchRead(BRACELET_TOUCH_PIN)
  );

  Serial.print(
    "Touch ON threshold: "
  );

  Serial.println(
    TOUCH_ON_THRESHOLD
  );

  Serial.print(
    "Touch OFF threshold: "
  );

  Serial.println(
    TOUCH_OFF_THRESHOLD
  );
}

// =====================================================
// Read averaged touch value
// =====================================================

uint16_t readTouchAverage() {
  uint32_t total = 0;

  for (
    int sampleIndex = 0;
    sampleIndex < TOUCH_SAMPLE_COUNT;
    sampleIndex++
  ) {
    total +=
      touchRead(
        BRACELET_TOUCH_PIN
      );

    delayMicroseconds(300);
  }

  return
    total /
    TOUCH_SAMPLE_COUNT;
}

// =====================================================
// Update touch state
// =====================================================

void updateBraceletTouch(
  bool &touchRaw,
  bool &braceletWorn,
  uint32_t &continuousTouchMs
) {
  unsigned long now =
    millis();

  currentTouchValue =
    readTouchAverage();

  // ---------------------------------------------------
  // Touch is not detected right now
  // ---------------------------------------------------

  if (!touchDetectedState) {
    /*
      The value drops when the foil is touched.
    */
    if (
      currentTouchValue <=
      TOUCH_ON_THRESHOLD
    ) {
      touchDetectedState = true;

      touchStartedMillis = now;
      touchLostStartedMillis = 0;

      Serial.println(
        "Touch detected"
      );
    }
  }

  // ---------------------------------------------------
  // Touch is detected right now
  // ---------------------------------------------------

  else {
    /*
      The value returned to the no-touch range.
    */
    if (
      currentTouchValue >=
      TOUCH_OFF_THRESHOLD
    ) {
      /*
        Start measuring how long there is no touch.
      */
      if (
        touchLostStartedMillis == 0
      ) {
        touchLostStartedMillis = now;
      }

      /*
        Only after 500ms without touch,
        the bracelet is considered removed.
      */
      if (
        now -
          touchLostStartedMillis >=
        TOUCH_LOST_CONFIRM_MS
      ) {
        touchDetectedState = false;
        braceletWornState = false;

        touchStartedMillis = 0;
        touchLostStartedMillis = 0;

        Serial.println(
          "Touch lost"
        );
      }
    }
    else {
      /*
        Touch is still present or returned before the timeout.
      */
      touchLostStartedMillis = 0;
    }
  }

  touchRaw =
    touchDetectedState;

  // ---------------------------------------------------
  // No touch
  // ---------------------------------------------------

  if (!touchDetectedState) {
    braceletWornState = false;

    continuousTouchMs = 0;
    braceletWorn = false;

    return;
  }

  // ---------------------------------------------------
  // Touch
  // ---------------------------------------------------

  if (
    touchStartedMillis == 0
  ) {
    touchStartedMillis = now;
  }

  continuousTouchMs =
    now -
    touchStartedMillis;

  /*
    Only after two continuous seconds,
    the bracelet is considered worn.
  */
  if (
    continuousTouchMs >=
    REQUIRED_TOUCH_MS
  ) {
    braceletWornState = true;
  }

  braceletWorn =
    braceletWornState;
}

// =====================================================
// Accelerometer initialization
// =====================================================

bool initializeAccelerometer() {
  Wire.begin(
    ACCEL_SDA_PIN,
    ACCEL_SCL_PIN
  );

  if (
    !accelerometer.begin(0x53)
  ) {
    Serial.println(
      "ERROR: ADXL345 not found at address 0x53"
    );

    return false;
  }

  accelerometer.setRange(
    ADXL345_RANGE_16_G
  );

  Serial.println(
    "ADXL345 initialized"
  );

  return true;
}

// =====================================================
// Accelerometer calibration
// =====================================================

void calibrateAccelerometer() {
  Serial.println();
  Serial.println(
    "Keep bracelet still"
  );

  Serial.println(
    "Calibrating accelerometer..."
  );

  const int sampleCount = 50;

  float sumX = 0.0f;
  float sumY = 0.0f;
  float sumZ = 0.0f;

  for (
    int sampleIndex = 0;
    sampleIndex < sampleCount;
    sampleIndex++
  ) {
    sensors_event_t event;

    accelerometer.getEvent(
      &event
    );

    sumX +=
      event.acceleration.x;

    sumY +=
      event.acceleration.y;

    sumZ +=
      event.acceleration.z;

    delay(30);
  }

  accelerationBaseX =
    sumX / sampleCount;

  accelerationBaseY =
    sumY / sampleCount;

  accelerationBaseZ =
    sumZ / sampleCount;

  Serial.println(
    "Accelerometer calibration completed"
  );

  Serial.print(
    "Base X: "
  );

  Serial.println(
    accelerationBaseX,
    3
  );

  Serial.print(
    "Base Y: "
  );

  Serial.println(
    accelerationBaseY,
    3
  );

  Serial.print(
    "Base Z: "
  );

  Serial.println(
    accelerationBaseZ,
    3
  );
}

// =====================================================
// Read movement state
// =====================================================

int readMovementState() {
  sensors_event_t event;

  accelerometer.getEvent(
    &event
  );

  float differenceX =
    event.acceleration.x -
    accelerationBaseX;

  float differenceY =
    event.acceleration.y -
    accelerationBaseY;

  float differenceZ =
    event.acceleration.z -
    accelerationBaseZ;

  currentMovementPower =
    sqrtf(
      differenceX * differenceX +
      differenceY * differenceY +
      differenceZ * differenceZ
    );

  if (
    currentMovementPower >=
    HIT_THRESHOLD
  ) {
    return 2;
  }

  if (
    currentMovementPower >=
    MOVEMENT_THRESHOLD
  ) {
    return 1;
  }

  return 0;
}

// =====================================================
// Connect to main ESP access point
// =====================================================

bool connectToMainEspAccessPoint() {
  if (
    WiFi.status() ==
    WL_CONNECTED
  ) {
    return true;
  }

  if (
    lastApAttemptMillis != 0 &&
    millis() -
      lastApAttemptMillis <
      AP_RETRY_INTERVAL_MS
  ) {
    return false;
  }

  lastApAttemptMillis =
    millis();

  Serial.println();
  Serial.println(
    "Connecting to main ESP local AP..."
  );

  Serial.print(
    "SSID: "
  );

  Serial.println(
    MAIN_AP_SSID
  );

  WiFi.disconnect(
    false,
    false
  );

  delay(300);

  WiFi.begin(
    MAIN_AP_SSID,
    MAIN_AP_PASSWORD
  );

  unsigned long connectionStartedMillis =
    millis();

  while (
    WiFi.status() !=
      WL_CONNECTED &&
    millis() -
      connectionStartedMillis <
      10000UL
  ) {
    Serial.print(".");
    delay(250);
  }

  Serial.println();

  if (
    WiFi.status() !=
    WL_CONNECTED
  ) {
    Serial.print(
      "WiFi connection failed. Status: "
    );

    Serial.println(
      static_cast<int>(
        WiFi.status()
      )
    );

    return false;
  }

  Serial.println(
    "Connected to main ESP local AP"
  );

  Serial.print(
    "Small ESP IP: "
  );

  Serial.println(
    WiFi.localIP()
  );

  Serial.print(
    "Small ESP MAC: "
  );

  Serial.println(
    WiFi.macAddress()
  );

  Serial.print(
    "WiFi channel: "
  );

  Serial.println(
    WiFi.channel()
  );

  return true;
}

// =====================================================
// Stop ESP-NOW
// =====================================================

void stopEspNow() {
  if (espNowReady) {
    esp_now_deinit();
  }

  espNowReady = false;

  sendInProgress = false;
  sendCallbackReceived = false;
  lastSendSucceeded = false;
}

// =====================================================
// Initialize ESP-NOW
// =====================================================

bool initializeEspNow() {
  if (espNowReady) {
    return true;
  }

  if (
    WiFi.status() !=
    WL_CONNECTED
  ) {
    Serial.println(
      "Cannot initialize ESP-NOW: WiFi disconnected"
    );

    return false;
  }

  Serial.println();
  Serial.println(
    "========== ESP-NOW INIT =========="
  );

  Serial.print(
    "Current WiFi channel: "
  );

  Serial.println(
    WiFi.channel()
  );

  esp_err_t initializationResult =
    esp_now_init();

  if (
    initializationResult !=
    ESP_OK
  ) {
    Serial.print(
      "esp_now_init failed. Code: "
    );

    Serial.println(
      static_cast<int>(
        initializationResult
      )
    );

    return false;
  }

  esp_now_register_send_cb(
    onEspNowDataSent
  );

  /*
    Remove the old peer if it already exists.
  */
  if (
    esp_now_is_peer_exist(
      broadcastAddress
    )
  ) {
    esp_now_del_peer(
      broadcastAddress
    );
  }

  esp_now_peer_info_t peerInformation = {};

  memcpy(
    peerInformation.peer_addr,
    broadcastAddress,
    6
  );

  /*
    Channel 0 uses the current Wi-Fi channel.
  */
  peerInformation.channel = 0;

  peerInformation.encrypt = false;

  peerInformation.ifidx =
    WIFI_IF_STA;

  esp_err_t peerResult =
    esp_now_add_peer(
      &peerInformation
    );

  if (
    peerResult !=
    ESP_OK
  ) {
    Serial.print(
      "esp_now_add_peer failed. Code: "
    );

    Serial.println(
      static_cast<int>(
        peerResult
      )
    );

    esp_now_deinit();

    return false;
  }

  espNowReady = true;

  sendInProgress = false;
  sendCallbackReceived = false;
  lastSendSucceeded = false;

  Serial.println(
    "Broadcast peer added"
  );

  Serial.println(
    "ESP-NOW ready"
  );

  Serial.println(
    "=================================="
  );

  return true;
}

// =====================================================
// Send bracelet message
// =====================================================

bool sendBraceletMessage(
  const BraceletMessage &message
) {
  if (
    !espNowReady ||
    WiFi.status() != WL_CONNECTED ||
    sendInProgress
  ) {
    return false;
  }

  sendInProgress = true;
  sendCallbackReceived = false;

  sendStartedMillis =
    millis();

  esp_err_t sendResult =
    esp_now_send(
      broadcastAddress,
      reinterpret_cast<const uint8_t *>(
        &message
      ),
      sizeof(message)
    );

  if (
    sendResult !=
    ESP_OK
  ) {
    sendInProgress = false;

    Serial.print(
      "esp_now_send failed. Code: "
    );

    Serial.println(
      static_cast<int>(
        sendResult
      )
    );

    return false;
  }

  return true;
}

// =====================================================
// Check ESP-NOW callback
// =====================================================

void checkSendCallback() {
  if (sendCallbackReceived) {
    sendCallbackReceived = false;

    Serial.print(
      "ESP-NOW callback: "
    );

    Serial.println(
      lastSendSucceeded
        ? "SUCCESS"
        : "FAILED"
    );
  }

  /*
    If the callback did not return in time,
    release the send lock.
  */
  if (
    sendInProgress &&
    millis() -
      sendStartedMillis >=
      SEND_CALLBACK_TIMEOUT_MS
  ) {
    sendInProgress = false;

    Serial.println(
      "ESP-NOW callback timeout"
    );
  }
}

// =====================================================
// Print bracelet state
// =====================================================

void printBraceletState(
  bool touchRaw,
  bool braceletWorn,
  uint32_t continuousTouchMs,
  bool sendStarted
) {
  Serial.print(
    "TouchValue="
  );

  Serial.print(
    currentTouchValue
  );

  Serial.print(
    " Touch="
  );

  Serial.print(
    touchRaw
      ? "YES"
      : "NO"
  );

  Serial.print(
    " worn="
  );

  Serial.print(
    braceletWorn
      ? "YES"
      : "NO"
  );

  Serial.print(
    " held="
  );

  Serial.print(
    continuousTouchMs
  );

  Serial.print(
    "ms movement="
  );

  Serial.print(
    braceletMessage.movementState
  );

  Serial.print(
    " power="
  );

  Serial.print(
    braceletMessage.movementPower,
    2
  );

  Serial.print(
    " send="
  );

  Serial.println(
    sendStarted
      ? "QUEUED"
      : "BUSY/ERROR"
  );
}

// =====================================================
// Setup
// =====================================================

void setup() {
  Serial.begin(
    115200
  );

  delay(1000);

  Serial.println();
  Serial.println(
    "=================================="
  );

  Serial.println(
    "SMALL ESP DIRECT TOUCH START"
  );

  Serial.println(
    "=================================="
  );

  /*
    No calibration is needed.
    The thresholds were set from real measurements.
  */
  initializeTouchSensor();

  if (
    !initializeAccelerometer()
  ) {
    while (true) {
      Serial.println(
        "Accelerometer initialization failed"
      );

      delay(1000);
    }
  }

  calibrateAccelerometer();

  WiFi.persistent(
    false
  );

  WiFi.mode(
    WIFI_STA
  );

  /*
    Disabling sleep mode improves ESP-NOW stability.
  */
  WiFi.setSleep(
    false
  );

  WiFi.disconnect(
    false,
    false
  );

  delay(300);

  /*
    Connect to the main ESP's Access Point.
  */
  while (
    !connectToMainEspAccessPoint()
  ) {
    delay(200);
  }

  /*
    After both ESPs are on the same channel,
    start ESP-NOW.
  */
  while (
    !initializeEspNow()
  ) {
    Serial.println(
      "ESP-NOW retry in one second"
    );

    delay(1000);
  }

  Serial.println();
  Serial.println(
    "Bracelet ready"
  );
}

// =====================================================
// Loop
// =====================================================

void loop() {
  checkSendCallback();

  // ---------------------------------------------------
  // Reconnect if Wi-Fi was lost
  // ---------------------------------------------------

  if (
    WiFi.status() !=
    WL_CONNECTED
  ) {
    Serial.println(
      "Main ESP AP connection lost"
    );

    stopEspNow();

    while (
      !connectToMainEspAccessPoint()
    ) {
      delay(200);
    }

    while (
      !initializeEspNow()
    ) {
      delay(1000);
    }

    lastSendMillis =
      millis();

    return;
  }

  // ---------------------------------------------------
  // Wait until next transmission
  // ---------------------------------------------------

  if (
    millis() -
      lastSendMillis <
    SEND_INTERVAL_MS
  ) {
    delay(1);
    return;
  }

  /*
    Do not send a new message before
    the previous message has finished.
  */
  if (sendInProgress) {
    delay(1);
    return;
  }

  lastSendMillis =
    millis();

  // ---------------------------------------------------
  // Read touch sensor
  // ---------------------------------------------------

  bool touchRaw = false;
  bool braceletWorn = false;
  uint32_t continuousTouchMs = 0;

  updateBraceletTouch(
    touchRaw,
    braceletWorn,
    continuousTouchMs
  );

  // ---------------------------------------------------
  // Build message
  // ---------------------------------------------------

  braceletMessage.version =
    MESSAGE_VERSION;

  braceletMessage.sequence =
    ++messageSequence;

  braceletMessage.touchRaw =
    touchRaw;

  braceletMessage.braceletWorn =
    braceletWorn;

  braceletMessage.continuousTouchMs =
    continuousTouchMs;

  braceletMessage.movementState =
    readMovementState();

  braceletMessage.movementPower =
    currentMovementPower;

  // ---------------------------------------------------
  // Send with ESP-NOW
  // ---------------------------------------------------

  bool sendStarted =
    sendBraceletMessage(
      braceletMessage
    );

  printBraceletState(
    touchRaw,
    braceletWorn,
    continuousTouchMs,
    sendStarted
  );
}