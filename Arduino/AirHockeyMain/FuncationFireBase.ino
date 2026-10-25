#include "Global.h"

bool networkAvailable = false;

// ---------- WiFi ----------
void connect() {
  /*
    AP_STA keeps the local bracelet link available even when
    the home router or the Internet is unavailable.
  */
  WiFi.mode(WIFI_AP_STA);

  bool apStarted = WiFi.softAP(
    BRACELET_AP_SSID,
    BRACELET_AP_PASSWORD
  );

  Serial.print(
    "Bracelet local AP: "
  );
  Serial.println(
    apStarted ? "READY" : "FAILED"
  );

  Serial.print(
    "Bracelet AP IP: "
  );
  Serial.println(
    WiFi.softAPIP()
  );

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  Serial.print(
    "Connecting to WiFi"
  );

  unsigned long startedAt =
    millis();

  const unsigned long timeoutMs =
    12000UL;

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - startedAt < timeoutMs
  ) {
    delay(300);
    Serial.print(".");
  }

  networkAvailable =
    WiFi.status() == WL_CONNECTED;

  if (networkAvailable) {
    Serial.println(
      "\nWiFi connected!"
    );

    Serial.print("IP: ");
    Serial.println(
      WiFi.localIP()
    );
  }
  else {
    Serial.println(
      "\nNo external WiFi. Bracelet ESP-NOW remains available."
    );
  }
}

// ---------- Time ----------
void init_time() {
  if (!networkAvailable) {
    Serial.println(
      "Time sync skipped - offline mode"
    );
    return;
  }

  configTime(
    0,
    0,
    "pool.ntp.org",
    "time.nist.gov"
  );

  Serial.print(
    "Waiting for time"
  );

  unsigned long startedAt =
    millis();

  while (
    time(nullptr) < 1670000000 &&
    millis() - startedAt < 10000UL
  ) {
    Serial.print(".");
    delay(300);
  }

  Serial.println(
    time(nullptr) >= 1670000000
      ? " Time set!"
      : " Time sync timeout"
  );
}

// ---------- Firebase ----------
void init_firebase() {
  if (!networkAvailable) {
    Serial.println(
      "Firebase skipped - offline mode"
    );
    return;
  }

  config.api_key = API_KEY;
  config.timeout.serverResponse = 30000;
  config.timeout.wifiReconnect = 30000;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback =
    tokenStatusCallback;

  Firebase.begin(
    &config,
    &auth
  );

  Firebase.reconnectWiFi(true);

  unsigned long startedAt =
    millis();

  while (
    !Firebase.ready() &&
    millis() - startedAt < 15000UL
  ) {
    Serial.println(
      "Waiting for Firebase..."
    );
    delay(500);
  }

  if (!Firebase.ready()) {
    Serial.println(
      "Firebase unavailable; continuing"
    );
    return;
  }

  fbdo.setBSSLBufferSize(
    8192,
    2048
  );

  Serial.println(
    "Firebase Ready!"
  );
}

// ---------- Helpers ----------
bool firestoreGetDoc(const String& path, String& payloadOut) {
  if (!Firebase.ready()) return false;

  if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, DATABASE_ID, path.c_str())) {
    payloadOut = fbdo.payload();
    return true;
  }

  Serial.print("GET failed: ");
  Serial.println(fbdo.errorReason());
  return false;
}

bool firestorePatchRaw(const String& path, const String& rawJson, const String& updateMask) {
  if (!Firebase.ready()) return false;

  if (Firebase.Firestore.patchDocument(
        &fbdo,
        FIREBASE_PROJECT_ID,
        DATABASE_ID,
        path.c_str(),
        rawJson.c_str(),
        updateMask.c_str()
      )) {
    return true;
  }

  Serial.print("PATCH failed: ");
  Serial.println(fbdo.errorReason());
  return false;
}

bool firestoreDeleteDoc(const String& path) {
  if (!Firebase.ready()) return false;

  if (Firebase.Firestore.deleteDocument(&fbdo, FIREBASE_PROJECT_ID, DATABASE_ID, path.c_str())) {
    return true;
  }

  Serial.print("DELETE failed: ");
  Serial.println(fbdo.errorReason());
  return false;
}

// ---------- Old debug function ----------
void get_document(const char* path) {
  String payload;
  if (firestoreGetDoc(String(path), payload)) {
    Serial.println(payload);
    lcd_clear();
    lcd_print_text(20, 20, "Doc loaded");
  }
}

// ---------- Live game write ----------
void send_game_data(int table, int p1, int p2, int speed, int timeSec, const String& playerName) {
  FirebaseJson content;

  content.set("fields/id/stringValue", "game1");
  content.set("fields/table_number/integerValue", table);
  content.set("fields/player1_score/integerValue", p1);
  content.set("fields/player2_score/integerValue", p2);
  content.set("fields/puck_speed/integerValue", speed);
  content.set("fields/game_time_seconds/integerValue", timeSec);
  content.set("fields/opponent_type/stringValue", "robot");
  content.set("fields/opponent_name/stringValue", "Robot");

  if (p1 >= 7 || p2 >= 7) {
    content.set("fields/status/stringValue", "completed");
    content.set("fields/is_active/booleanValue", false);

    if (p1 >= 7) {
      content.set("fields/winner/stringValue", playerName);
      content.set("fields/last_goal_by/stringValue", "player1");
    } else {
      content.set("fields/winner/stringValue", "Robot");
      content.set("fields/last_goal_by/stringValue", "player2");
    }
  } else {
    content.set("fields/status/stringValue", "in_progress");
    content.set("fields/is_active/booleanValue", true);

    if (p1 > p2) {
      content.set("fields/last_goal_by/stringValue", "player1");
    } else if (p2 > p1) {
      content.set("fields/last_goal_by/stringValue", "player2");
    } else {
      content.set("fields/last_goal_by/stringValue", "none");
    }

    content.set("fields/winner/stringValue", "");
  }

  content.set("fields/updated_at/stringValue", String((unsigned long)time(nullptr)));

  String documentPath = "ESP32Data/game1";

  if (Firebase.Firestore.patchDocument(
        &fbdo,
        FIREBASE_PROJECT_ID,
        DATABASE_ID,
        documentPath.c_str(),
        content.raw(),
        "id,table_number,player1_score,player2_score,puck_speed,game_time_seconds,opponent_type,opponent_name,status,is_active,last_goal_by,winner,updated_at"
      )) {
    Serial.println("Game data sent!");
  } else {
    Serial.println(fbdo.errorReason());
  }
}