#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include <SPI.h>
#include <DMD2.h>
#include <fonts/Arial14.h>

// WiFi credentials
const char* ssid = "YASH1234";
const char* password = "YASH1234";

// Display configuration
const int WIDTH = 1;
const int HEIGHT = 1;
const uint8_t* FONT = Arial14;

// DMD2 Display
SPIDMD dmd(WIDTH, HEIGHT, 5, 16, 2, 12);
ESP8266WebServer server(80);

// Message scroll variables
String currentMessage = "Welcome!";
int scrollX = WIDTH * 32; // Start from right edge
unsigned long lastScrollTime = 0;
int scrollDelay = 100; // ms

// Web Interface (HTML + JS)
const char MAIN_page[] PROGMEM = R"=====( 
<!DOCTYPE html>
<html>
<head>
  <title>Matrix Display Controller</title>
  <script>
    function sendMessage() {
      const msg = document.getElementById("msg").value;
      const speed = document.getElementById("speed").value;
      const xhr = new XMLHttpRequest();
      xhr.open("GET", "/update?msg=" + encodeURIComponent(msg) + "&speed=" + speed, true);
      xhr.send();
    }

    function updatePreview() {
      const msg = document.getElementById("msg").value;
      document.getElementById("preview").innerText = msg;
    }
  </script>
</head>
<body>
  <center>
    <h2>Matrix Display Controller</h2>
    <textarea id="msg" rows="3" cols="30" oninput="updatePreview()">Enter your text here</textarea><br><br>
    Scroll Speed: <input type="range" id="speed" min="50" max="500" value="100"> ms<br><br>
    <button onclick="sendMessage()">Update Display</button><br><br>
    <h3>Live Preview:</h3>
    <pre id="preview">Enter your text here</pre>
  </center>
</body>
</html>
)=====";

// Function to decode URL-encoded strings (handles %20 and others)
String urlDecode(String input) {
  String decoded = "";
  char temp[] = "0x00";
  unsigned int len = input.length();
  unsigned int i = 0;

  while (i < len) {
    char c = input.charAt(i);
    if (c == '+') {
      decoded += ' ';
    } else if (c == '%' && i + 2 < len) {
      temp[2] = input.charAt(i + 1);
      temp[3] = input.charAt(i + 2);
      decoded += (char) strtol(temp, NULL, 16);
      i += 2;
    } else {
      decoded += c;
    }
    i++;
  }
  return decoded;
}

// HTTP Handlers
void handleRoot() {
  server.send(200, "text/html", MAIN_page);
}

void handleUpdate() {
  if (server.hasArg("msg")) {
    currentMessage = urlDecode(server.arg("msg"));
    scrollX = WIDTH * 32;
  }
  if (server.hasArg("speed")) {
    scrollDelay = constrain(server.arg("speed").toInt(), 50, 500);
  }
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/update", handleUpdate);
  server.begin();
  Serial.println("HTTP server started");

  dmd.begin();
  dmd.setBrightness(255);
  dmd.selectFont(FONT);
  scrollX = WIDTH * 32;
}

void loop() {
  server.handleClient();

  unsigned long now = millis();
  if (now - lastScrollTime >= scrollDelay) {
    dmd.clearScreen();

    int charWidth = 8; // approx for Arial14
    int textWidth = currentMessage.length() * charWidth;
    int displayWidth = WIDTH * 32;

    if (textWidth <= displayWidth) {
      // Center text
      int centerX = (displayWidth - textWidth) / 2;
      dmd.drawString(centerX, 0, currentMessage.c_str());
    } else {
      // Scroll text
      dmd.drawString(scrollX, 0, currentMessage.c_str());
      scrollX--;
      if (scrollX < -textWidth) {
        scrollX = displayWidth;
      }
    }

    lastScrollTime = now;
  }
}