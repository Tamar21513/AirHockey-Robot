#include "Global.h"

// ---------------------------------------------------
// Utilities
// ---------------------------------------------------

String readRequestBody() {
  if (!server.hasArg("plain")) return "";
  return server.arg("plain");
}

void sendJson(int statusCode, const String& json) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(statusCode, "application/json; charset=utf-8", json);
}

void sendOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(200, "text/plain", "");
}

String getQueryParam(const String& key) {
  if (server.hasArg(key)) return server.arg(key);
  return "";
}

String sanitizeEmail(String email) {
  email.replace("@", "_at_");
  email.replace(".", "_dot_");
  email.replace("+", "_plus_");
  return email;
}

// ---------------------------------------------------
// Firestore -> simple JSON helpers
// ---------------------------------------------------

String docStringField(FirebaseJson& json, const String& fieldName, const String& def = "") {
  FirebaseJsonData result;
  String path = "fields/" + fieldName + "/stringValue";
  if (json.get(result, path)) return result.stringValue;
  return def;
}

int docIntField(FirebaseJson& json, const String& fieldName, int def = 0) {
  FirebaseJsonData result;
  String path = "fields/" + fieldName + "/integerValue";
  if (json.get(result, path)) return result.intValue;
  return def;
}

bool docBoolField(FirebaseJson& json, const String& fieldName, bool def = false) {
  FirebaseJsonData result;
  String path = "fields/" + fieldName + "/booleanValue";
  if (json.get(result, path)) return result.boolValue;
  return def;
}

String sha256String(const String& input) {
  byte shaResult[32];
  mbedtls_sha256_context ctx;

  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts_ret(&ctx, 0);
  mbedtls_sha256_update_ret(&ctx, (const unsigned char*)input.c_str(), input.length());
  mbedtls_sha256_finish_ret(&ctx, shaResult);
  mbedtls_sha256_free(&ctx);

  char output[65];
  for (int i = 0; i < 32; i++) {
    sprintf(output + (i * 2), "%02X", shaResult[i]);
  }
  output[64] = '\0';

  return String(output);
}

// ---------------------------------------------------
// Generic stored JSON docs
// Idea: for lists/dashboards we use cache documents in Firebase
// that the ESP32 reads and returns to React
// ---------------------------------------------------

bool getJsonCacheDoc(const String& docPath, String& jsonOut) {
  if (!Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, DATABASE_ID, docPath.c_str())) {
    Serial.println(fbdo.errorReason());
    return false;
  }

  FirebaseJson json;
  json.setJsonData(fbdo.payload());

  FirebaseJsonData result;
  if (json.get(result, "fields/json/stringValue")) {
    jsonOut = result.stringValue;

    // Remove unnecessary escape characters
    jsonOut.replace("\\n", "");
    jsonOut.replace("\\r", "");
    jsonOut.replace("\\t", " ");
    jsonOut.replace("\\\"", "\"");

    jsonOut.trim();

    return true;
  }

  return false;
}

bool setJsonCacheDoc(const String& docPath, const String& rawJson) {
  FirebaseJson content;
  content.set("fields/json/stringValue", rawJson);

  return Firebase.Firestore.patchDocument(
    &fbdo,
    FIREBASE_PROJECT_ID,
    DATABASE_ID,
    docPath.c_str(),
    content.raw(),
    "json"
  );
}

// ---------------------------------------------------
// /game + game code helpers
// ---------------------------------------------------

bool isPlayableStatus(String status) {
  status.toLowerCase();
  status.trim();
  return status == "scheduled";
}

String normalizeGameCode(String code) {
  code.trim();
  if (code.startsWith("game_")) {
    code = code.substring(5);
  }
  return code;
}

String normalizeGameId(String value) {
  value.trim();
  if (value.length() == 0) return "";
  if (value.startsWith("game_")) return value;
  return "game_" + value;
}

String jsonEscape(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  value.replace("\n", " ");
  value.replace("\r", " ");
  return value;
}

String buildGameResponse(String docId, FirebaseJson& json, bool wrapSuccess = true) {
  String gameCode = docStringField(json, "game_code", "");

  if (gameCode == "" && docId.startsWith("game_")) {
    gameCode = docId.substring(5);
  }

  String status = docStringField(json, "status", "scheduled");

  String response = "";
  if (wrapSuccess) {
    response += "{\"success\":true,\"game\":{";
  } else {
    response += "{";
  }

  response += "\"id\":\"" + jsonEscape(docId) + "\",";
  response += "\"game_id\":\"" + jsonEscape(docId) + "\",";
  response += "\"game_code\":\"" + jsonEscape(gameCode) + "\",";

  response += "\"date\":\"" + jsonEscape(docStringField(json, "date", "")) + "\",";
  response += "\"time_slot\":\"" + jsonEscape(docStringField(json, "time_slot", "")) + "\",";

  response += "\"table_number\":" + String(docIntField(json, "table_number", 1)) + ",";
  response += "\"player1_email\":\"" + jsonEscape(docStringField(json, "player1_email", "")) + "\",";
  response += "\"player1_name\":\"" + jsonEscape(docStringField(json, "player1_name", "Player")) + "\",";

  response += "\"opponent_type\":\"" + jsonEscape(docStringField(json, "opponent_type", "robot")) + "\",";
  response += "\"opponent_name\":\"" + jsonEscape(docStringField(json, "opponent_name", "Robot")) + "\",";

  response += "\"payment_source\":\"" + jsonEscape(docStringField(json, "payment_source", "")) + "\",";
  response += "\"payment_method\":\"" + jsonEscape(docStringField(json, "payment_method", "")) + "\",";
  response += "\"bonus_id\":\"" + jsonEscape(docStringField(json, "bonus_id", "")) + "\",";

  response += "\"player1_score\":" + String(docIntField(json, "player1_score", 0)) + ",";
  response += "\"player2_score\":" + String(docIntField(json, "player2_score", 0)) + ",";
  response += "\"game_time_seconds\":" + String(docIntField(json, "game_time_seconds", 0)) + ",";
  response += "\"puck_speed\":" + String(docIntField(json, "puck_speed", 0)) + ",";
  response += "\"is_active\":" + String(docBoolField(json, "is_active", status == "in_progress") ? "true" : "false") + ",";
  response += "\"last_goal_by\":\"" + jsonEscape(docStringField(json, "last_goal_by", "none")) + "\",";

  response += "\"skill_level\":\"" + jsonEscape(docStringField(json, "skill_level", "")) + "\",";
  response += "\"game_mode\":\"" + jsonEscape(docStringField(json, "game_mode", "")) + "\",";
  response += "\"status\":\"" + jsonEscape(status) + "\",";
  response += "\"winner\":\"" + jsonEscape(docStringField(json, "winner", "")) + "\"";

  if (wrapSuccess) {
    response += "}}";
  } else {
    response += "}";
  }

  return response;
}

bool loadGameByDocId(String docId, FirebaseJson& outJson) {
  if (docId.length() == 0) return false;
  String path = "games/" + docId;

  if (!Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, DATABASE_ID, path.c_str())) {
    return false;
  }

  outJson.setJsonData(fbdo.payload());
  return true;
}

bool loadGameByCode(String code, String &foundDocId, FirebaseJson &foundDoc) {
  code.trim();

  Serial.println("----- loadGameByCode -----");
  Serial.print("Input code: ");
  Serial.println(code);

  if (code.length() == 0) {
    Serial.println("Code is empty");
    return false;
  }

  if (code.startsWith("game_")) {
    code = code.substring(5);
  }

  Serial.print("Clean code: ");
  Serial.println(code);

  // =====================================================
  // Attempt 1: search by document name
  // Example: games/game_578085
  // =====================================================

  String docIdByName = "game_" + code;
  String docPath = "games/" + docIdByName;

  Serial.print("Try direct doc path: ");
  Serial.println(docPath);

  if (Firebase.Firestore.getDocument(
        &fbdo,
        FIREBASE_PROJECT_ID,
        DATABASE_ID,
        docPath.c_str()
      )) {
    Serial.println("Found by document id");

    foundDocId = docIdByName;
    foundDoc.setJsonData(fbdo.payload());

    return true;
  }

  Serial.print("Direct doc not found: ");
  Serial.println(fbdo.errorReason());

  // =====================================================
  // Attempt 2: search by the game_code field
  // Example: game_code == "578085"
  //
  // Important:
  // With this library you do not write structuredQuery/...
  // only from / where / limit
  // =====================================================

  FirebaseJson query;

  query.set("from/[0]/collectionId", "games");

  query.set("where/fieldFilter/field/fieldPath", "game_code");
  query.set("where/fieldFilter/op", "EQUAL");
  query.set("where/fieldFilter/value/stringValue", code);

  query.set("limit", 1);

  String queryDebug;
  query.toString(queryDebug, true);

  Serial.println("Structured query object:");
  Serial.println(queryDebug);

  Serial.print("Try runQuery by game_code = ");
  Serial.println(code);

  if (!Firebase.Firestore.runQuery(
        &fbdo,
        FIREBASE_PROJECT_ID,
        DATABASE_ID,
        "",
        &query
      )) {
    Serial.print("runQuery failed: ");
    Serial.println(fbdo.errorReason());
    return false;
  }

  String payload = fbdo.payload();

  Serial.println("runQuery payload:");
  Serial.println(payload);

  if (payload.indexOf("\"document\"") == -1) {
    Serial.println("No document in query result");
    return false;
  }

  // =====================================================
  // Parse the runQuery result
  // =====================================================

  FirebaseJsonArray arr;
  arr.setJsonArrayData(payload);

  FirebaseJsonData itemData;
  arr.get(itemData, 0);

  if (!itemData.success) {
    Serial.println("Failed to read first array item");
    return false;
  }

  FirebaseJson item;
  item.setJsonData(itemData.stringValue);

  FirebaseJsonData docData;
  item.get(docData, "document");

  if (!docData.success) {
    Serial.println("Failed to read document object");
    return false;
  }

  FirebaseJson doc;
  doc.setJsonData(docData.stringValue);

  FirebaseJsonData nameData;
  doc.get(nameData, "name");

  if (!nameData.success) {
    Serial.println("Failed to read document name");
    return false;
  }

  String fullName = nameData.stringValue;
  foundDocId = fullName.substring(fullName.lastIndexOf("/") + 1);

  Serial.print("Found by game_code. Doc id: ");
  Serial.println(foundDocId);

  foundDoc = doc;

  return true;
}

void handleGetGame() {
  if (server.method() == HTTP_OPTIONS) {
    sendOptions();
    return;
  }

  if (!Firebase.ready()) {
    sendJson(500, "{\"error\":\"firebase_not_ready\"}");
    return;
  }

  String code = getQueryParam("code");
  code.trim();

  if (code.length() > 0) {
    FirebaseJson gameJson;
    String foundDocId = "";

    if (!loadGameByCode(code, foundDocId, gameJson)) {
      sendJson(404, "{\"success\":false,\"message\":\"game_not_found\"}");
      return;
    }

    sendJson(200, buildGameResponse(foundDocId, gameJson, false));
    return;
  }

  if (!Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, DATABASE_ID, "ESP32Data/game1")) {
    sendJson(404, "{\"error\":\"game_not_found\"}");
    return;
  }

  FirebaseJson json;
  json.setJsonData(fbdo.payload());

  String response = "{";
  response += "\"id\":\"" + docStringField(json, "id", "game1") + "\",";
  response += "\"table_number\":" + String(docIntField(json, "table_number", 1)) + ",";
  response += "\"player1_score\":" + String(docIntField(json, "player1_score", 0)) + ",";
  response += "\"player2_score\":" + String(docIntField(json, "player2_score", 0)) + ",";
  response += "\"game_time_seconds\":" + String(docIntField(json, "game_time_seconds", 0)) + ",";
  response += "\"puck_speed\":" + String(docIntField(json, "puck_speed", 0)) + ",";
  response += "\"is_active\":" + String(docBoolField(json, "is_active", false) ? "true" : "false") + ",";
  response += "\"last_goal_by\":\"" + docStringField(json, "last_goal_by", "none") + "\",";
  response += "\"status\":\"" + docStringField(json, "status", "idle") + "\",";
  response += "\"updated_at\":\"" + docStringField(json, "updated_at", "") + "\",";
  response += "\"winner\":\"" + docStringField(json, "winner", "") + "\"";
  response += "}";

  sendJson(200, response);
}

void handlePostGame() {
  if (server.method() == HTTP_OPTIONS) {
    sendOptions();
    return;
  }

  String body = readRequestBody();
  if (body.length() == 0) {
    sendJson(400, "{\"success\":false,\"message\":\"missing_body\"}");
    return;
  }

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    sendJson(400, "{\"success\":false,\"message\":\"invalid_json\"}");
    return;
  }

  String gameCode = String(doc["game_code"] | "");
  gameCode = normalizeGameCode(gameCode);

  if (gameCode.length() == 0) {
    gameCode = String((unsigned long)millis());
  }

  String gameId = String(doc["id"] | "");
  if (gameId.length() == 0) {
    gameId = String(doc["game_id"] | "");
  }
  if (gameId.length() == 0) {
    gameId = normalizeGameId(gameCode);
  }
  gameId = normalizeGameId(gameId);

  String date = doc["date"] | "";
  String timeSlot = doc["time_slot"] | "";
  String playerEmail = doc["player_email"] | "";
  String paymentSource = doc["payment_source"] | "";
  String paymentMethod = doc["payment_method"] | "";
  String bonusId = doc["bonus_id"] | "";
  String playerName = doc["player_name"] | "Player";
  String opponentType = doc["opponent_type"] | "robot";
  String opponentName = doc["opponent_name"] | "Robot";

  FirebaseJson content;
  content.set("fields/id/stringValue", gameId);
  content.set("fields/game_code/stringValue", gameCode);
  content.set("fields/date/stringValue", date);
  content.set("fields/time_slot/stringValue", timeSlot);
  content.set("fields/skill_level/stringValue", "");
  content.set("fields/game_mode/stringValue", "");
  content.set("fields/player1_email/stringValue", playerEmail);
  content.set("fields/payment_source/stringValue", paymentSource);
  content.set("fields/payment_method/stringValue", paymentMethod);
  content.set("fields/bonus_id/stringValue", bonusId);
  content.set("fields/status/stringValue", "scheduled");
  content.set("fields/player1_score/integerValue", 0);
  content.set("fields/player2_score/integerValue", 0);
  content.set("fields/player1_name/stringValue", playerName);
  content.set("fields/opponent_type/stringValue", opponentType);
  content.set("fields/opponent_name/stringValue", opponentName);
  content.set("fields/winner/stringValue", "");
  content.set("fields/table_number/integerValue", 1);
  content.set("fields/game_time_seconds/integerValue", 0);
  content.set("fields/puck_speed/integerValue", 0);
  content.set("fields/last_goal_by/stringValue", "none");
  content.set("fields/is_active/booleanValue", false);

  String path = "games/" + gameId;

  bool ok = Firebase.Firestore.patchDocument(
    &fbdo,
    FIREBASE_PROJECT_ID,
    DATABASE_ID,
    path.c_str(),
    content.raw(),
    "id,game_code,date,time_slot,skill_level,game_mode,player1_email,player1_name,opponent_type,opponent_name,payment_source,payment_method,bonus_id,status,player1_score,player2_score,winner,table_number,game_time_seconds,puck_speed,last_goal_by,is_active"
  );

  if (!ok) {
    sendJson(500, "{\"success\":false,\"message\":\"firebase_write_failed\"}");
    return;
  }

  String response = "{";
  response += "\"success\":true,";
  response += "\"game_id\":\"" + gameId + "\",";
  response += "\"id\":\"" + gameId + "\",";
  response += "\"game_code\":\"" + gameCode + "\"";
  response += "}";

  sendJson(200, response);
}

void handleGetGameByCode() {
  if (server.method() == HTTP_OPTIONS) {
    sendOptions();
    return;
  }

  String code = getQueryParam("code");
  code = normalizeGameCode(code);

  if (code.length() == 0) {
    sendJson(400, "{\"success\":false,\"message\":\"missing_game_code\"}");
    return;
  }

  FirebaseJson gameJson;
  String foundDocId = "";

  if (!loadGameByCode(code, foundDocId, gameJson)) {
    sendJson(404, "{\"success\":false,\"message\":\"game_not_found\"}");
    return;
  }

  String status = docStringField(gameJson, "status", "scheduled");

  if (!isPlayableStatus(status)) {
    String response = "{";
    response += "\"success\":false,";
    response += "\"message\":\"game_already_used\",";
    response += "\"status\":\"" + jsonEscape(status) + "\"";
    response += "}";
    sendJson(409, response);
    return;
  }

  sendJson(200, buildGameResponse(foundDocId, gameJson, true));
}

void handleGetGameItem() {
  if (server.method() == HTTP_OPTIONS) {
    sendOptions();
    return;
  }

  String id = getQueryParam("id");
  id = normalizeGameId(id);

  if (id.length() == 0) {
    sendJson(400, "{\"success\":false,\"message\":\"missing_id\"}");
    return;
  }

  FirebaseJson gameJson;
  if (!loadGameByDocId(id, gameJson)) {
    sendJson(404, "{\"success\":false,\"message\":\"game_not_found\"}");
    return;
  }

  sendJson(200, buildGameResponse(id, gameJson, true));
}

void handleStartGame() {
  if (server.method() == HTTP_OPTIONS) {
    sendOptions();
    return;
  }

  String body = readRequestBody();
  if (body.length() == 0) {
    sendJson(400, "{\"success\":false,\"message\":\"missing_body\"}");
    return;
  }

  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, body)) {
    sendJson(400, "{\"success\":false,\"message\":\"invalid_json\"}");
    return;
  }

  String gameCode = String(doc["game_code"] | "");
  gameCode = normalizeGameCode(gameCode);

  String gameId = String(doc["game_id"] | "");
  if (gameId.length() == 0) {
    gameId = String(doc["id"] | "");
  }

  gameId.trim();

  FirebaseJson existingGame;
  String foundDocId = "";

  bool found = false;
  if (gameId.length() > 0) {
    gameId = normalizeGameId(gameId);
    found = loadGameByDocId(gameId, existingGame);
    if (found) foundDocId = gameId;
  }

  if (!found && gameCode.length() > 0) {
    found = loadGameByCode(gameCode, foundDocId, existingGame);
  }

  if (!found) {
    sendJson(404, "{\"success\":false,\"message\":\"game_not_found\"}");
    return;
  }

  String currentStatus = docStringField(existingGame, "status", "scheduled");

  if (!isPlayableStatus(currentStatus)) {
    String response = "{";
    response += "\"success\":false,";
    response += "\"message\":\"game_already_used\",";
    response += "\"status\":\"" + jsonEscape(currentStatus) + "\"";
    response += "}";
    sendJson(409, response);
    return;
  }

  if (gameCode.length() == 0) {
    gameCode = docStringField(existingGame, "game_code", "");
  }
  if (gameCode.length() == 0 && foundDocId.startsWith("game_")) {
    gameCode = foundDocId.substring(5);
  }

  int skillLevel = int(doc["skill_level"] | 1);
  String gameMode = doc["game_mode"] | "first_to_7";

  FirebaseJson content;
  content.set("fields/status/stringValue", "in_progress");
  content.set("fields/skill_level/integerValue", skillLevel);
  content.set("fields/game_mode/stringValue", gameMode);
  content.set("fields/game_code/stringValue", gameCode);
  content.set("fields/player1_score/integerValue", 0);
  content.set("fields/player2_score/integerValue", 0);
  content.set("fields/winner/stringValue", "");
  content.set("fields/is_active/booleanValue", true);
  content.set("fields/game_time_seconds/integerValue", 0);
  content.set("fields/last_goal_by/stringValue", "none");

  String documentPath = "games/" + foundDocId;

  bool ok = Firebase.Firestore.patchDocument(
    &fbdo,
    FIREBASE_PROJECT_ID,
    DATABASE_ID,
    documentPath.c_str(),
    content.raw(),
    "status,skill_level,game_mode,game_code,player1_score,player2_score,winner,is_active,game_time_seconds,last_goal_by"
  );

  if (!ok) {
    sendJson(500, "{\"success\":false,\"message\":\"failed_to_start_game\"}");
    return;
  }

  String response = "{";
  response += "\"success\":true,";
  response += "\"message\":\"game_started\",";
  response += "\"game\":{";
  response += "\"id\":\"" + jsonEscape(foundDocId) + "\",";
  response += "\"game_id\":\"" + jsonEscape(foundDocId) + "\",";
  response += "\"game_code\":\"" + jsonEscape(gameCode) + "\",";
  response += "\"skill_level\":" + String(skillLevel) + ",";
  response += "\"game_mode\":\"" + jsonEscape(gameMode) + "\",";
  response += "\"status\":\"in_progress\",";
  response += "\"is_active\":true";
  response += "}}";

  sendJson(200, response);
}

// ---------------------------------------------------
// subscriptions
// There are two levels here:
// 1. Global subscription list: cache doc
// 2. User subscription: doc by email
// ---------------------------------------------------

void handleGetSubscriptions() {
  String jsonOut;
  if (getJsonCacheDoc("public_api/subscriptions", jsonOut)) {
    sendJson(200, jsonOut);
  } else {
    sendJson(200, "[]");
  }
}

void handlePostSubscriptions() {
  String body = readRequestBody();
  if (body.length() == 0) {
    sendJson(400, "{\"success\":false}");
    return;
  }

  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, body)) {
    sendJson(400, "{\"success\":false,\"message\":\"invalid_json\"}");
    return;
  }

  String id = "sub_" + String((unsigned long)millis());

  FirebaseJson content;
  content.set("fields/id/stringValue", id);
  content.set("fields/name/stringValue", String(doc["name"] | ""));
  content.set("fields/type/stringValue", String(doc["type"] | "monthly"));
  content.set("fields/price/integerValue", int(doc["price"] | 0));
  content.set("fields/games_included/integerValue", int(doc["games_included"] | 0));
  content.set("fields/description/stringValue", String(doc["description"] | ""));
  content.set("fields/benefits/stringValue", String(doc["benefits"] | ""));
  content.set("fields/is_active/booleanValue", bool(doc["is_active"] | true));

  String path = "subscriptions/" + id;

  bool ok = Firebase.Firestore.patchDocument(
    &fbdo,
    FIREBASE_PROJECT_ID,
    DATABASE_ID,
    path.c_str(),
    content.raw(),
    "id,name,type,price,games_included,description,benefits,is_active"
  );

  sendJson(ok ? 200 : 500, ok ? "{\"success\":true}" : "{\"success\":false}");
}

void handlePutSubscription() {
  String id = server.arg("id");
  if (id.length() == 0) {
    sendJson(400, "{\"success\":false,\"message\":\"missing_id\"}");
    return;
  }

  String body = readRequestBody();
  DynamicJsonDocument doc(1024);

  if (deserializeJson(doc, body)) {
    sendJson(400, "{\"success\":false}");
    return;
  }

  FirebaseJson content;
  if (doc.containsKey("name")) content.set("fields/name/stringValue", String(doc["name"] | ""));
  if (doc.containsKey("type")) content.set("fields/type/stringValue", String(doc["type"] | ""));
  if (doc.containsKey("price")) content.set("fields/price/integerValue", int(doc["price"] | 0));
  if (doc.containsKey("games_included")) content.set("fields/games_included/integerValue", int(doc["games_included"] | 0));
  if (doc.containsKey("description")) content.set("fields/description/stringValue", String(doc["description"] | ""));
  if (doc.containsKey("benefits")) content.set("fields/benefits/stringValue", String(doc["benefits"] | ""));
  if (doc.containsKey("is_active")) content.set("fields/is_active/booleanValue", bool(doc["is_active"] | true));

  String path = "subscriptions/" + id;
  bool ok = Firebase.Firestore.patchDocument(
    &fbdo,
    FIREBASE_PROJECT_ID,
    DATABASE_ID,
    path.c_str(),
    content.raw(),
    "name,type,price,games_included,description,benefits,is_active"
  );

  sendJson(ok ? 200 : 500, ok ? "{\"success\":true}" : "{\"success\":false}");
}

void handleDeleteSubscription() {
  String id = server.arg("id");
  if (id.length() == 0) {
    sendJson(400, "{\"success\":false,\"message\":\"missing_id\"}");
    return;
  }

  String path = "subscriptions/" + id;
  bool ok = Firebase.Firestore.deleteDocument(&fbdo, FIREBASE_PROJECT_ID, DATABASE_ID, path.c_str());

  sendJson(ok ? 200 : 500, ok ? "{\"success\":true}" : "{\"success\":false}");
}

// ---------------------------------------------------
// user subscription
// ---------------------------------------------------

void handleGetUserSubscription() {
  String email = getQueryParam("email");
  if (email.length() == 0) {
    sendJson(400, "{\"error\":\"missing_email\"}");
    return;
  }

  String docId = sanitizeEmail(email);
  String path = "user_subscriptions/" + docId;

  if (!Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, DATABASE_ID, path.c_str())) {
    sendJson(200, "null");
    return;
  }

  FirebaseJson json;
  json.setJsonData(fbdo.payload());

  String response = "{";
  response += "\"user_email\":\"" + docStringField(json, "user_email", email) + "\",";
  response += "\"subscription_id\":\"" + docStringField(json, "subscription_id", "") + "\",";
  response += "\"subscription_name\":\"" + docStringField(json, "subscription_name", "") + "\",";
  response += "\"games_remaining\":" + String(docIntField(json, "games_remaining", 0)) + ",";
  response += "\"end_date\":\"" + docStringField(json, "end_date", "") + "\",";
  response += "\"status\":\"" + docStringField(json, "status", "inactive") + "\"";
  response += "}";

  sendJson(200, response);
}

void handlePostSubscribe() {
  String body = readRequestBody();
  DynamicJsonDocument doc(1024);

  if (deserializeJson(doc, body)) {
    sendJson(400, "{\"success\":false,\"message\":\"invalid_json\"}");
    return;
  }

  String email = doc["user_email"] | "";
  if (email.length() == 0) {
    sendJson(400, "{\"success\":false,\"message\":\"missing_email\"}");
    return;
  }

  String docId = sanitizeEmail(email);
  String path = "user_subscriptions/" + docId;

  FirebaseJson content;
  content.set("fields/user_email/stringValue", email);
  content.set("fields/subscription_id/stringValue", String(doc["subscription_id"] | ""));
  content.set("fields/subscription_name/stringValue", String(doc["subscription_name"] | ""));
  content.set("fields/type/stringValue", String(doc["type"] | ""));
  content.set("fields/price/integerValue", int(doc["price"] | 0));
  content.set("fields/games_remaining/integerValue", int(doc["games_remaining"] | 0));
  content.set("fields/payment_method/stringValue", String(doc["payment_method"] | ""));
  content.set("fields/start_date/stringValue", String(doc["start_date"] | ""));
  content.set("fields/end_date/stringValue", String(doc["end_date"] | ""));
  content.set("fields/status/stringValue", String(doc["status"] | "active"));

  bool ok = Firebase.Firestore.patchDocument(
    &fbdo,
    FIREBASE_PROJECT_ID,
    DATABASE_ID,
    path.c_str(),
    content.raw(),
    "user_email,subscription_id,subscription_name,type,price,games_remaining,payment_method,start_date,end_date,status"
  );

  sendJson(ok ? 200 : 500, ok ? "{\"success\":true}" : "{\"success\":false}");
}

// ---------------------------------------------------
// Generic cache-backed endpoints
// The idea: ready JSON documents in Firestore under public_api/
// ---------------------------------------------------

void handleGetHomeData() {
  String email = getQueryParam("email");
  String docId = "public_api/home_" + sanitizeEmail(email);
  String jsonOut;
  if (getJsonCacheDoc(docId, jsonOut)) sendJson(200, jsonOut);
  else sendJson(200, "{\"stats\":{\"games\":0,\"highScore\":0,\"rank\":\"-\"}}");
}

void handleGetHistory() {
  String email = getQueryParam("email");
  String docId = "public_api/history_" + sanitizeEmail(email);
  String jsonOut;
  if (getJsonCacheDoc(docId, jsonOut)) sendJson(200, jsonOut);
  else sendJson(200, "{\"games\":[],\"scores\":[]}");
}

void handleGetLeaderboard() {
  String jsonOut;
  if (getJsonCacheDoc("public_api/leaderboard", jsonOut)) sendJson(200, jsonOut);
  else sendJson(200, "{\"leaderboard\":[],\"myRank\":null}");
}

void handleGetAdminDashboard() {
  String jsonOut;
  if (getJsonCacheDoc("public_api/admin_dashboard", jsonOut)) sendJson(200, jsonOut);
  else sendJson(200, "{\"stats\":{\"games\":0,\"users\":0,\"revenue\":0,\"activeSubs\":0},\"games\":[]}");
}

void handleGetPlayers() {
  String jsonOut;
  if (getJsonCacheDoc("public_api/players", jsonOut)) sendJson(200, jsonOut);
  else sendJson(200, "[]");
}

void handleGetHighScores() {
  String jsonOut;
  if (getJsonCacheDoc("public_api/high_scores", jsonOut)) sendJson(200, jsonOut);
  else sendJson(200, "[]");
}

void handleGetBonuses() {
  String jsonOut;
  if (getJsonCacheDoc("public_api/bonuses", jsonOut)) sendJson(200, jsonOut);
  else sendJson(200, "[]");
}

void handlePostBonus() {
  String body = readRequestBody();
  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, body)) {
    sendJson(400, "{\"success\":false}");
    return;
  }

  String id = "bonus_" + String((unsigned long)millis());

  FirebaseJson content;
  content.set("fields/id/stringValue", id);
  content.set("fields/user_email/stringValue", String(doc["user_email"] | ""));
  content.set("fields/user_name/stringValue", String(doc["user_name"] | ""));
  content.set("fields/type/stringValue", String(doc["type"] | "high_score"));
  content.set("fields/amount/integerValue", int(doc["amount"] | 0));
  content.set("fields/description/stringValue", String(doc["description"] | ""));
  content.set("fields/game_id/stringValue", String(doc["game_id"] | ""));
  content.set("fields/is_claimed/booleanValue", false);

  String path = "bonuses/" + id;

  bool ok = Firebase.Firestore.patchDocument(
    &fbdo,
    FIREBASE_PROJECT_ID,
    DATABASE_ID,
    path.c_str(),
    content.raw(),
    "id,user_email,user_name,type,amount,description,game_id,is_claimed"
  );

  sendJson(ok ? 200 : 500, ok ? "{\"success\":true}" : "{\"success\":false}");
}

void handleDeleteBonus() {
  String id = server.arg("id");
  if (id.length() == 0) {
    sendJson(400, "{\"success\":false,\"message\":\"missing_id\"}");
    return;
  }

  String path = "bonuses/" + id;
  bool ok = Firebase.Firestore.deleteDocument(&fbdo, FIREBASE_PROJECT_ID, DATABASE_ID, path.c_str());

  sendJson(ok ? 200 : 500, ok ? "{\"success\":true}" : "{\"success\":false}");
}

void handleGetGamesByDate() {
  String date = getQueryParam("date");
  String docId = "public_api/games_" + date;
  docId.replace("-", "_");

  String jsonOut;
  if (getJsonCacheDoc(docId, jsonOut)) sendJson(200, jsonOut);
  else sendJson(200, "[]");
}

void handlePutGame() {
  String id = server.arg("id");
  if (id.length() == 0) {
    sendJson(400, "{\"success\":false,\"message\":\"missing_id\"}");
    return;
  }

  String body = readRequestBody();
  DynamicJsonDocument doc(1024);

  if (deserializeJson(doc, body)) {
    sendJson(400, "{\"success\":false}");
    return;
  }

  FirebaseJson content;

  if (doc.containsKey("time_slot")) {
    content.set("fields/time_slot/stringValue", String(doc["time_slot"] | ""));
  }

  if (doc.containsKey("skill_level")) {
    content.set("fields/skill_level/stringValue", String(doc["skill_level"] | ""));
  }

  if (doc.containsKey("status")) {
    content.set("fields/status/stringValue", String(doc["status"] | ""));
  }

  if (doc.containsKey("table_number")) {
    content.set("fields/table_number/integerValue", int(doc["table_number"] | 0));
  }

  if (doc.containsKey("winner")) {
    content.set("fields/winner/stringValue", String(doc["winner"] | ""));
  }

  String path = "games/" + id;

  bool ok = Firebase.Firestore.patchDocument(
    &fbdo,
    FIREBASE_PROJECT_ID,
    DATABASE_ID,
    path.c_str(),
    content.raw(),
    "time_slot,skill_level,status,table_number,winner"
  );

  sendJson(ok ? 200 : 500, ok ? "{\"success\":true}" : "{\"success\":false}");
}

void handleDeleteGame() {
  String id = server.arg("id");
  if (id.length() == 0) {
    sendJson(400, "{\"success\":false,\"message\":\"missing_id\"}");
    return;
  }

  String path = "games/" + id;
  bool ok = Firebase.Firestore.deleteDocument(&fbdo, FIREBASE_PROJECT_ID, DATABASE_ID, path.c_str());

  sendJson(ok ? 200 : 500, ok ? "{\"success\":true}" : "{\"success\":false}");
}

// --------------------------------------------------
// login
//----------------------------------------------------
void handleLogin() {
  String body = readRequestBody();

  if (body.length() == 0) {
    sendJson(400, "{\"success\":false,\"message\":\"missing_body\"}");
    return;
  }

  DynamicJsonDocument req(512);
  DeserializationError err = deserializeJson(req, body);

  if (err) {
    sendJson(400, "{\"success\":false,\"message\":\"invalid_json\"}");
    return;
  }

  String email = req["email"] | "";
  lcd_clear();
  lcd_print_text(20,20,email);
  String password = req["password"] | "";

  email.trim();
  password.trim();

  if (email.length() == 0 || password.length() == 0) {
    sendJson(400, "{\"success\":false,\"message\":\"Please enter email and password\"}");
    return;
  }

  String userDocId = sanitizeEmail(email);
  String path = "users/" + userDocId;

  if (!Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, DATABASE_ID, path.c_str())) {
    sendJson(401, "{\"success\":false,\"message\":\"User not found\"}");
    return;
  }

  FirebaseJson json;
  json.setJsonData(fbdo.payload());

  String dbEmail = docStringField(json, "email", "");
  String dbName = docStringField(json, "name", "User");
  String dbRole = docStringField(json, "role", "user");
  String dbPasswordHash = docStringField(json, "passwordHash", "");
  bool isActive = docBoolField(json, "isActive", true);

  if (!isActive) {
    sendJson(403, "{\"success\":false,\"message\":\"The user is blocked\"}");
    return;
  }

  String inputHash = sha256String(password);

  if (dbEmail != email || dbPasswordHash != inputHash) {
    sendJson(401, "{\"success\":false,\"message\":\"Wrong email or password\"}");
    return;
  }

  String response = "{";
  response += "\"success\":true,";
  response += "\"user\":{";
  response += "\"name\":\"" + dbName + "\",";
  response += "\"email\":\"" + dbEmail + "\",";
  response += "\"role\":\"" + dbRole + "\"";
  response += "}";
  response += "}";

  sendJson(200, response);
}


// ---------------------------------------------------
// user bonuses
// ---------------------------------------------------

void handleGetUserBonuses() {
  String email = getQueryParam("email");
  if (email.length() == 0) {
    sendJson(400, "{\"error\":\"missing_email\"}");
    return;
  }

  String docId = "public_api/bonuses_" + sanitizeEmail(email);
  String jsonOut;

  if (getJsonCacheDoc(docId, jsonOut)) {
    sendJson(200, jsonOut);
  } else {
    sendJson(200, "[]");
  }
}

// ---------------------------------------------------
// register
// ---------------------------------------------------

void handleRegister() {
  if (server.method() == HTTP_OPTIONS) {
    sendOptions();
    return;
  }

  String body = readRequestBody();
  if (body.length() == 0) {
    sendJson(400, "{\"success\":false,\"message\":\"missing_body\"}");
    return;
  }

  DynamicJsonDocument doc(2048);
  if (deserializeJson(doc, body)) {
    sendJson(400, "{\"success\":false,\"message\":\"invalid_json\"}");
    return;
  }

  String email = doc["email"] | "";
  String name = doc["name"] | "";
  String phone = doc["phone"] | "";
  String passwordHash = doc["passwordHash"] | "";
  String role = doc["role"] | "user";
  String createdAt = doc["createdAt"] | "";

  email.trim();
  name.trim();
  phone.trim();
  passwordHash.trim();

  if (email.length() == 0 || name.length() == 0 || phone.length() == 0 || passwordHash.length() == 0) {
    sendJson(400, "{\"success\":false,\"message\":\"missing_required_fields\"}");
    return;
  }

  String userDocId = sanitizeEmail(email);
  String path = "users/" + userDocId;

  if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, DATABASE_ID, path.c_str())) {
    sendJson(409, "{\"success\":false,\"message\":\"user_already_exists\"}");
    return;
  }

  FirebaseJson content;
  content.set("fields/createdAt/stringValue", createdAt);
  content.set("fields/email/stringValue", email);
  content.set("fields/isActive/booleanValue", true);
  content.set("fields/name/stringValue", name);
  content.set("fields/passwordHash/stringValue", passwordHash);
  content.set("fields/phone/stringValue", phone);
  content.set("fields/role/stringValue", role);

  bool ok = Firebase.Firestore.patchDocument(
    &fbdo,
    FIREBASE_PROJECT_ID,
    DATABASE_ID,
    path.c_str(),
    content.raw(),
    "createdAt,email,isActive,name,passwordHash,phone,role"
  );

  if (!ok) {
    sendJson(500, "{\"success\":false,\"message\":\"firebase_write_failed\"}");
    return;
  }

  String response = "{";
  response += "\"success\":true,";
  response += "\"user\":{";
  response += "\"name\":\"" + jsonEscape(name) + "\",";
  response += "\"email\":\"" + jsonEscape(email) + "\",";
  response += "\"phone\":\"" + jsonEscape(phone) + "\",";
  response += "\"role\":\"" + jsonEscape(role) + "\",";
  response += "\"isActive\":true";
  response += "}}";

  sendJson(200, response);
}

// ---------------------------------------------------
// Init server
// ---------------------------------------------------

void init_react_server() {

  server.on("/game", HTTP_GET, handleGetGame);
  server.on("/game", HTTP_POST, handlePostGame);
  server.on("/game", HTTP_OPTIONS, sendOptions);

  server.on("/game-by-code", HTTP_GET, handleGetGameByCode);
  server.on("/game-by-code", HTTP_OPTIONS, sendOptions);

  server.on("/start-game", HTTP_POST, handleStartGame);
  server.on("/start-game", HTTP_OPTIONS, sendOptions);

  server.on("/subscriptions", HTTP_GET, handleGetSubscriptions);
  server.on("/subscriptions", HTTP_POST, handlePostSubscriptions);
  server.on("/subscriptions", HTTP_OPTIONS, sendOptions);

  server.on("/subscription", HTTP_PUT, handlePutSubscription);
  server.on("/subscription", HTTP_DELETE, handleDeleteSubscription);
  server.on("/subscription", HTTP_OPTIONS, sendOptions);

  server.on("/user-subscription", HTTP_GET, handleGetUserSubscription);
  server.on("/user-subscription", HTTP_OPTIONS, sendOptions);

  server.on("/user-bonuses", HTTP_GET, handleGetUserBonuses);
  server.on("/user-bonuses", HTTP_OPTIONS, sendOptions);

  server.on("/subscribe", HTTP_POST, handlePostSubscribe);
  server.on("/subscribe", HTTP_OPTIONS, sendOptions);

  server.on("/home-data", HTTP_GET, handleGetHomeData);
  server.on("/history", HTTP_GET, handleGetHistory);
  server.on("/leaderboard", HTTP_GET, handleGetLeaderboard);
  server.on("/admin-dashboard", HTTP_GET, handleGetAdminDashboard);
  server.on("/players", HTTP_GET, handleGetPlayers);
  server.on("/high-scores", HTTP_GET, handleGetHighScores);
  server.on("/bonuses", HTTP_GET, handleGetBonuses);
  server.on("/bonuses", HTTP_POST, handlePostBonus);
  server.on("/games", HTTP_GET, handleGetGamesByDate);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/login", HTTP_OPTIONS, sendOptions);

  server.on("/register", HTTP_POST, handleRegister);
  server.on("/register", HTTP_OPTIONS, sendOptions);

  server.on("/bonus", HTTP_DELETE, handleDeleteBonus);
  server.on("/bonus", HTTP_OPTIONS, sendOptions);

  server.on("/game-item", HTTP_GET, handleGetGameItem);
  server.on("/game-item", HTTP_PUT, handlePutGame);
  server.on("/game-item", HTTP_DELETE, handleDeleteGame);
  server.on("/game-item", HTTP_OPTIONS, sendOptions);
  
  server.on("/move", HTTP_GET, handleMoveCommand);
  server.on("/move", HTTP_POST, handleMoveCommand);
  server.on("/move", HTTP_OPTIONS, sendOptions);

  server.on("/robot-status", HTTP_GET, handleRobotStatus);
  server.on("/robot-status", HTTP_OPTIONS, sendOptions);

  // Raw OpenCV detections arrive here.
  // All motion calculations and decisions are performed by the ESP32.
  server.on("/puck-update", HTTP_POST, handlePuckUpdate);
  server.on("/puck-update", HTTP_OPTIONS, sendOptions);

  server.begin();
  Serial.println("React bridge server started!");
}

void handle_react_server() {
  server.handleClient();
}