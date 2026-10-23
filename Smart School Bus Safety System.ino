#include <SPI.h>
#include <MFRC522.h>
#include <TinyGPSPlus.h>

// ================ PIN CONFIGURATION ================
#define GSM_RX 16
#define GSM_TX 17

#define GPS_RX 32
#define GPS_TX 33

#define RFID_SS   5
#define RFID_RST  22
#define RFID_SCK  18
#define RFID_MISO 19
#define RFID_MOSI 23

#define BUZZER 25

HardwareSerial GSM(2);
HardwareSerial GPS(1);

MFRC522 rfid(RFID_SS, RFID_RST);
TinyGPSPlus gps;

// ================ CONTACTS ==========================
const char DRIVER_PHONE[] = "+919767535752";

// Replace placeholders with actual authorised numbers.
const char SCHOOL_PHONE[] = "+919545665099";

// ================ TIMING ============================
const unsigned long DRIVER_WAIT_MS = 60000UL;
const unsigned long GPS_STALE_MS = 10000UL;

// ================ STUDENT DATABASE =================
// Replace sample UIDs and parent numbers with actual data.
// UID DD:B9:4F:06 is included below.

struct Student {
  const char *uid;
  const char *name;
  const char *parentPhone;
  bool onBus;
};

Student students[] = {
  {"DD:B9:4F:06", "Sahil", "+918421508424", false},
  {"AA:BB:CC:DD", "Student 2", "+919876546538", false},
  {"11:22:33:44", "Student 3", "+919767724587", false}
};

const size_t STUDENT_COUNT =
  sizeof(students) / sizeof(students[0]);

// ================ SYSTEM STATE ======================
bool tripActive = false;
bool driverAlertActive = false;
bool driverAcknowledged = false;
bool escalationDone = false;
bool buzzerActive = false;

unsigned long driverAlertStarted = 0;

String lastGsmStatus = "Not tested";
String lastAlertStatus = "No alert";

// ================ UID HELPERS =======================
String normaliseUID(String uid) {
  uid.replace(":", "");
  uid.replace(" ", "");
  uid.replace("-", "");
  uid.toUpperCase();
  return uid;
}

String readUID() {
  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }

  uid.toUpperCase();
  return uid;
}

int findStudent(String uid) {
  uid = normaliseUID(uid);

  for (size_t i = 0; i < STUDENT_COUNT; i++) {
    if (uid == normaliseUID(students[i].uid)) {
      return (int)i;
    }
  }

  return -1;
}

int countOnBus() {
  int count = 0;

  for (size_t i = 0; i < STUDENT_COUNT; i++) {
    if (students[i].onBus) count++;
  }

  return count;
}

String studentsOnBus() {
  String result = "";

  for (size_t i = 0; i < STUDENT_COUNT; i++) {
    if (!students[i].onBus) continue;

    if (result.length()) result += ", ";
    result += students[i].name;
  }

  return result.length() ? result : "None";
}

// ================ BUZZER =============================
void startBuzzer() {
  buzzerActive = true;
  digitalWrite(BUZZER, HIGH);
}

void stopBuzzer() {
  buzzerActive = false;
  digitalWrite(BUZZER, LOW);
}

// ================ GPS ================================
void updateGPS() {
  while (GPS.available()) {
    gps.encode(GPS.read());
  }
}

bool gpsFixValid() {
  return gps.location.isValid() &&
         gps.location.age() <= GPS_STALE_MS &&
         gps.location.lat() >= -90.0 &&
         gps.location.lat() <= 90.0 &&
         gps.location.lng() >= -180.0 &&
         gps.location.lng() <= 180.0;
}

String getLocationText() {
  updateGPS();

  if (!gpsFixValid()) {
    return "GPS location unavailable. Check bus manually.";
  }

  String text = "GPS location:\n";
  text += "Latitude: " + String(gps.location.lat(), 6);
  text += "\nLongitude: " + String(gps.location.lng(), 6);
  text += "\nhttps://maps.google.com/?q=";
  text += String(gps.location.lat(), 6);
  text += ",";
  text += String(gps.location.lng(), 6);

  return text;
}

// ================ GSM HELPERS ========================
String readGSM(unsigned long timeoutMs) {
  String response = "";
  unsigned long start = millis();

  while (millis() - start < timeoutMs) {
    updateGPS();

    while (GSM.available()) {
      char c = GSM.read();
      response += c;

      if (response.indexOf("+CMS ERROR") >= 0 ||
          response.indexOf("+CME ERROR") >= 0 ||
          response.indexOf("\r\nERROR\r\n") >= 0) {
        return response;
      }

      if (response.indexOf("OK") >= 0 ||
          response.indexOf(">") >= 0 ||
          response.indexOf("+CMGS:") >= 0) {
        return response;
      }
    }

    delay(1);
  }

  return response;
}

String gsmCommand(const String &command,
                  unsigned long timeoutMs = 2500) {
  while (GSM.available()) GSM.read();

  GSM.println(command);
  return readGSM(timeoutMs);
}

bool gsmRegistered() {
  String response = gsmCommand("AT+CREG?", 2500);

  return response.indexOf("+CREG: 0,1") >= 0 ||
         response.indexOf("+CREG: 0,5") >= 0 ||
         response.indexOf("+CREG: 1,1") >= 0 ||
         response.indexOf("+CREG: 1,5") >= 0;
}

bool sendSMS(const char *number, String message) {
  Serial.print("SMS attempt to: ");
  Serial.println(number);

  if (!gsmRegistered()) {
    lastGsmStatus = "Not registered";
    Serial.println("GSM ERROR: Network not registered.");
    return false;
  }

  String mode = gsmCommand("AT+CMGF=1", 3000);

  if (mode.indexOf("OK") < 0) {
    lastGsmStatus = "SMS text mode failed";
    Serial.println("GSM ERROR: CMGF failed.");
    return false;
  }

  while (GSM.available()) GSM.read();

  GSM.print("AT+CMGS=\"");
  GSM.print(number);
  GSM.println("\"");

  String prompt = readGSM(6000);

  if (prompt.indexOf(">") < 0) {
    lastGsmStatus = "SMS prompt failed";
    Serial.println("GSM ERROR: No SMS prompt.");
    return false;
  }

  GSM.print(message);
  GSM.write(26);

  String result = "";
  unsigned long start = millis();

  while (millis() - start < 30000UL) {
    updateGPS();

    while (GSM.available()) {
      result += (char)GSM.read();
    }

    if (result.indexOf("+CMGS:") >= 0 &&
        result.indexOf("OK") >= 0) {
      lastGsmStatus = "Accepted by modem";
      Serial.println("SMS accepted by modem.");
      return true;
    }

    if (result.indexOf("+CMS ERROR") >= 0 ||
        result.indexOf("+CME ERROR") >= 0 ||
        result.indexOf("\r\nERROR\r\n") >= 0) {
      lastGsmStatus = "SMS failed";
      Serial.println(result);
      return false;
    }

    delay(1);
  }

  lastGsmStatus = "SMS timeout";
  Serial.println("GSM ERROR: SMS result timeout.");
  return false;
}

void sendAlert(const char *number, String reason) {
  String message = "SMART SCHOOL BUS SAFETY ALERT\n";
  message += reason;
  message += "\nTrip status: ";
  message += tripActive ? "ACTIVE" : "ENDED";
  message += "\n";
  message += getLocationText();

  sendSMS(number, message);
}

// ================ TRIP MANAGEMENT ====================
void startTrip() {
  if (tripActive) {
    Serial.println("Trip already active.");
    return;
  }

  for (size_t i = 0; i < STUDENT_COUNT; i++) {
    students[i].onBus = false;
  }

  tripActive = true;
  driverAlertActive = false;
  driverAcknowledged = false;
  escalationDone = false;

  stopBuzzer();

  lastAlertStatus = "No alert";
  Serial.println("TRIP STARTED.");
  Serial.println("Scan student RFID cards.");
}

void startDriverAlert() {
  if (countOnBus() == 0) return;

  driverAlertActive = true;
  driverAcknowledged = false;
  escalationDone = false;
  driverAlertStarted = millis();

  startBuzzer();

  Serial.println("SAFETY ALERT: Student record remains on bus.");
  Serial.print("Students: ");
  Serial.println(studentsOnBus());

  sendAlert(
    DRIVER_PHONE,
    "Student record remains ON BUS. Check the bus immediately."
  );

  lastAlertStatus = "Driver alert initiated";
}

void endTrip() {
  if (!tripActive) {
    Serial.println("No active trip.");
    return;
  }

  tripActive = false;
  Serial.println("TRIP ENDED.");

  if (countOnBus() > 0) {
    startDriverAlert();
  } else {
    stopBuzzer();
    lastAlertStatus = "Trip completed; no unresolved students";
    Serial.println("All recorded students exited.");
  }
}

// Driver acknowledgment confirms the alert was seen,
// NOT that the bus is physically empty.
void acknowledgeDriver() {
  if (!driverAlertActive) {
    Serial.println("No pending driver alert.");
    return;
  }

  driverAcknowledged = true;

  Serial.println("DRIVER ACKNOWLEDGED ALERT.");
  Serial.println("Physically check the bus.");
  Serial.println("Record each confirmed exit by RFID.");
  Serial.println("Acknowledgment alone does not clear the alert.");
}

// ================ RFID PROCESSING ====================
void processRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = readUID();

  Serial.print("\nScanned UID: ");
  Serial.println(uid);

  int index = findStudent(uid);

  if (index < 0) {
    Serial.println("WRONG BUS / UNAUTHORISED CARD");
  } else if (tripActive) {
    Student &s = students[index];

    if (!s.onBus) {
      s.onBus = true;

      Serial.print("BOARDING RECORDED: ");
      Serial.println(s.name);
    } else {
      s.onBus = false;

      Serial.print("EXIT RECORDED: ");
      Serial.println(s.name);

      if (countOnBus() == 0 && driverAlertActive) {
        driverAlertActive = false;
        driverAcknowledged = false;
        escalationDone = false;

        stopBuzzer();

        lastAlertStatus = "Resolved by RFID exit records";
        Serial.println("ALERT RESOLVED: All student records exited.");
      }
    }
  } else {
    Serial.println("Registered card; start trip first using N.");
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// ================ ALERT ESCALATION ===================
void updateAlerts() {
  if (!driverAlertActive) return;

  if (countOnBus() == 0) {
    driverAlertActive = false;
    stopBuzzer();
    lastAlertStatus = "Resolved";
    Serial.println("ALERT RESOLVED.");
    return;
  }

  if (!escalationDone &&
      millis() - driverAlertStarted >= DRIVER_WAIT_MS) {

    escalationDone = true;

    Serial.println("ESCALATION: Student exit still unconfirmed.");

    sendAlert(
      SCHOOL_PHONE,
      "URGENT: Student exit remains unconfirmed after driver alert."
    );

    for (size_t i = 0; i < STUDENT_COUNT; i++) {
      if (!students[i].onBus) continue;

      sendAlert(
        students[i].parentPhone,
        String("Student exit is not confirmed: ") +
        students[i].name +
        ". Please contact the school."
      );
    }

    lastAlertStatus = "Escalation attempted";
    Serial.println("School/parent escalation attempts completed.");
  }
}

// ================ STATUS & DIAGNOSTICS ===============
void printStatus() {
  updateGPS();

  Serial.println("\n===== SYSTEM STATUS =====");

  Serial.print("Trip active: ");
  Serial.println(tripActive ? "YES" : "NO");

  Serial.print("Students recorded on bus: ");
  Serial.println(studentsOnBus());

  Serial.print("Driver acknowledged: ");
  Serial.println(driverAcknowledged ? "YES" : "NO");

  Serial.print("GPS: ");
  Serial.println(gpsFixValid() ? "VALID GPS FIX" : "WAITING / UNAVAILABLE");

  if (gpsFixValid()) {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Satellites: ");
    Serial.println(gps.satellites.value());
  }

  Serial.print("GSM: ");
  Serial.println(lastGsmStatus);

  Serial.print("Alert: ");
  Serial.println(lastAlertStatus);

  Serial.print("Buzzer: ");
  Serial.println(buzzerActive ? "ON" : "OFF");

  Serial.println("=========================");
}

void gsmDiagnostics() {
  Serial.println("\n===== GSM DIAGNOSTICS =====");

  Serial.println("AT:");
  Serial.println(gsmCommand("AT"));

  Serial.println("SIM status:");
  Serial.println(gsmCommand("AT+CPIN?"));

  Serial.println("Signal:");
  Serial.println(gsmCommand("AT+CSQ"));

  Serial.println("Network registration:");
  Serial.println(gsmCommand("AT+CREG?"));

  Serial.println("===========================");
}

// ================ SERIAL COMMANDS ====================
void processSerial() {
  if (!Serial.available()) return;

  String command = Serial.readStringUntil('\n');
  command.trim();
  command.toUpperCase();

  if (command == "N") {
    startTrip();
  } else if (command == "E") {
    endTrip();
  } else if (command == "A") {
    acknowledgeDriver();
  } else if (command == "D") {
    printStatus();
  } else if (command == "G") {
    Serial.println(getLocationText());
  } else if (command == "B") {
    Serial.println("Buzzer test.");
    startBuzzer();
    delay(1000);
    stopBuzzer();
  } else if (command == "R") {
    gsmDiagnostics();
  } else {
    Serial.println("Commands: N E A D G B R");
  }
}

// ================ SETUP ===============================
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);

  pinMode(BUZZER, OUTPUT);
  stopBuzzer();

  SPI.begin(RFID_SCK, RFID_MISO, RFID_MOSI, RFID_SS);
  rfid.PCD_Init();

  GSM.begin(9600, SERIAL_8N1, GSM_RX, GSM_TX);
  GPS.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  Serial.println("\nSMART SCHOOL BUS SAFETY SYSTEM");
  Serial.println("ESP32 + RFID + GPS + GSM + BUZZER");
  Serial.println("Heartbeat output: DISABLED");
  Serial.println("GPS source: NEO-6M ONLY");
  Serial.println("Registered UID: DD:B9:4F:06");

  Serial.println("\nCommands:");
  Serial.println("N = Start trip");
  Serial.println("E = End trip");
  Serial.println("A = Driver acknowledgment");
  Serial.println("D = System status");
  Serial.println("G = GPS location");
  Serial.println("B = Buzzer test");
  Serial.println("R = GSM diagnostics");
}

// ================ MAIN LOOP ==========================
void loop() {
  updateGPS();
  processRFID();
  updateAlerts();
  processSerial();
}
