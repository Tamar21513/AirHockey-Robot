#include "Global.h"


bool startAutomaticCamera() {
  Serial.println(
    "Python live-pair camera loop is managed automatically"
  );

  return true;
}


bool stopAutomaticCamera() {
  Serial.println(
    "Python live-pair camera loop remains active in the background"
  );

  return true;
}
