#include "Global.h"

volatile bool braceletPacketReceived = false;
volatile bool braceletWorn = false;
volatile bool braceletTouchRaw = false;
volatile uint32_t braceletContinuousTouchMs = 0;
volatile uint32_t braceletLastPacketMillis = 0;
volatile int braceletMovementState = 0;
volatile float braceletMovementPower = 0.0f;

portMUX_TYPE braceletMux = portMUX_INITIALIZER_UNLOCKED;

void onBraceletDataReceived(const uint8_t *senderMac, const uint8_t *incomingData, int dataLength) {
  if (dataLength != sizeof(BraceletMessage)) {
    return;
  }

  BraceletMessage message;
  memcpy(
    &message,
    incomingData,
    sizeof(message)
  );

  if (
    message.version !=
    BRACELET_MESSAGE_VERSION
  ) {
    return;
  }

  portENTER_CRITICAL_ISR(
    &braceletMux
  );

  braceletPacketReceived = true;
  braceletTouchRaw = message.touchRaw;
  braceletWorn = message.braceletWorn;
  braceletContinuousTouchMs =
    message.continuousTouchMs;
  braceletMovementState =
    message.movementState;
  braceletMovementPower =
    message.movementPower;
  braceletLastPacketMillis =
    millis();

  portEXIT_CRITICAL_ISR(
    &braceletMux
  );
}


void initEspNowBracelet() {
  Serial.println();
  Serial.println(
    "========== ESP-NOW BRACELET =========="
  );
  if (esp_now_init() != ESP_OK) {
    Serial.println(
      "ESP-NOW initialization failed"
    );
    return;
  }

  esp_now_register_recv_cb(
    onBraceletDataReceived
  );

  Serial.print(
    "Main ESP STA MAC: "
  );
  Serial.println(
    WiFi.macAddress()
  );

  Serial.print(
    "Main ESP AP MAC: "
  );
  Serial.println(
    WiFi.softAPmacAddress()
  );

  Serial.println(
    "Waiting for bracelet broadcasts"
  );
}


bool isBraceletConnected() {
  uint32_t lastPacket;

  portENTER_CRITICAL(
    &braceletMux
  );

  lastPacket =
    braceletLastPacketMillis;

  portEXIT_CRITICAL(
    &braceletMux
  );

  if (lastPacket == 0) {
    return false;
  }

  return (
    millis() - lastPacket <=
    BRACELET_PACKET_TIMEOUT_MS
  );
}


bool isBraceletReadyForGame() {
  bool worn;
  bool rawTouch;
  uint32_t heldMs;

  portENTER_CRITICAL(
    &braceletMux
  );

  worn = braceletWorn;
  rawTouch = braceletTouchRaw;
  heldMs = braceletContinuousTouchMs;

  portEXIT_CRITICAL(
    &braceletMux
  );

  return (
    isBraceletConnected() &&
    rawTouch &&
    worn &&
    heldMs >=
      BRACELET_REQUIRED_TOUCH_MS
  );
}


void resetBraceletAuthorization() {
  portENTER_CRITICAL(
    &braceletMux
  );

  braceletWorn = false;
  braceletTouchRaw = false;
  braceletContinuousTouchMs = 0;

  portEXIT_CRITICAL(
    &braceletMux
  );
}


uint32_t getBraceletContinuousTouchMs() {
  uint32_t heldMs;

  portENTER_CRITICAL(
    &braceletMux
  );

  heldMs =
    braceletContinuousTouchMs;

  portEXIT_CRITICAL(
    &braceletMux
  );

  return heldMs;
}
