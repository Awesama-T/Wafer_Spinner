
// Further project details at https://randomnerdtutorials.com/esp8266-nodemcu-access-point-ap-web-server/
// Import required libraries

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Servo.h>
Servo ESC;  // create servo object to control the ESC
const char* ssid = "ESP8266-Access-Point";
const char* password = "123456789";
const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";
int s = 0;
int h = 0;
// D6(GPIO 12) is for ESC output PWM.
// D7(GPIO 13) is for starting the rotation.
// D1(GPIO 05) is for tachometer.
bool flag = LOW;
float rpm_mes = 0;  // the measured rpm
int rpm_entr = 0;   // the desired rpm
int rotn_dur = 0;   // the desired time of rotation.
int time_lft = 0;   // the time left for rotation
int pin = 13;       // to start rotation
int pwm = 12;       // esc output PWM
int tach_mtr = 5;   // input pin of tachometer.
int counter = 50;
int val = 0;
long time1 = 0;  // the time of motor starting
long time2 = 0;  // will store the instantaneous time
int compare = 0;
long temp1 = 0;
long temp2 = 0;
int temp3 = 0;


void tachometer() {
  if (analogRead(tach_mtr) < 950) {
    temp1 = millis();
    delay(30);
    while (analogRead(tach_mtr) > 300) {
      Serial.print("almost done");
    }
    temp2 = millis();
  }
  temp3 = temp2 - temp1;
  rpm_mes = 60 / (temp3 * 0.0002);
  s = rpm_mes;
}




// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
  
<style>
html {
font-family: Arial;
display: inline-block;
margin: 0px auto;
text-align: center;
}
h2 { font-size: 3.0rem; } // font size of the heading
p { font-size: 1.5rem; } // size of the whole paragraph; placeholders
.dht-labels{
font-size: 1.5rem; // the font size of labels; speed & time
vertical-align:middle;
padding-bottom: 15px;
}
</style>
</head>
<body>
                <h2>Osama's Server</h2>
<form action="/get">
                Speed desired (Rpm): <input type="text" name="input1">
<input type="submit" value="Submit">
</form><br>
<form action="/get">
                Time desired (Sec): <input type="text" name="input2">
<input type="submit" value="Submit">
</form><br>

                
<p>
                <span class="dht-labels">Speed (Rpm):</span> 
                <span id="Speed">%Speed%</span>   
</p>
<p>
    
                <span class="dht-labels">Time left:</span> 
                <span id="humidity">%HUMIDITY%</span>
</p>
</body>
<script>

setInterval(function ( ) {
var xhttp = new XMLHttpRequest();
xhttp.onreadystatechange = function() {
if (this.readyState == 4 && this.status == 200) {
                    document.getElementById("Speed").innerHTML = this.responseText;
}
};
                    xhttp.open("GET", "/Speed", true);
xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
var xhttp = new XMLHttpRequest();
xhttp.onreadystatechange = function() {
if (this.readyState == 4 && this.status == 200) {
                    document.getElementById("humidity").innerHTML = this.responseText;
}
};
                    xhttp.open("GET", "/humidity", true);
xhttp.send();
}, 1000 ) ;
</script>
</html>)rawliteral";

void notFound(AsyncWebServerRequest* request) {
  request->send(404, "text/plain", "Not found");
}
// Replaces placeholder with actual values
String processor(const String& var) {
  if (var == "Speed") {
    return String(s);
  } else if (var == "HUMIDITY") {
    return String(h);
  }
  return String();
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  ESC.attach(pwm, 500, 2500);  // (pin, min pulse width, max pulse width in microseconds)
  ESC.write(50);               // Send the arming signal to the ESC.
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/Speed", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", String(s).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", String(h).c_str());
  });
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest* request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      rpm_entr = inputMessage.toInt();
    }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
      rotn_dur = inputMessage.toInt();

    } else {
      inputMessage = "No message sent";
      inputParam = "none";
    }

    Serial.println(rpm_entr);
    Serial.println(rotn_dur);

    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" + inputParam + ") with value: " + inputMessage + "<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  server.begin();
  // Start server

  pinMode(LED_BUILTIN, OUTPUT);  // Initialize the LED_BUILTIN pin as an output
  pinMode(pin, INPUT);           //it will receive the signal to turn ON.
  analogWrite(pin, 0);           // pulled low

  pinMode(tach_mtr, INPUT);  //it will receive input from tachometer.
  analogWrite(tach_mtr, 0);  // pulled low

  ESC.attach(pwm);  //Specify here the pin number on which the signal pin of ESC is connected.//equivalent to pin D5
  delay(3000);      // ESC initialization delay.
};

void loop() {
  val = analogRead(pin);  // momentarily hold pin high to start the rotation.
if (val>50)
{
  flag = ~ flag; 
  delay(100);
}
  if (flag)
  {
    h = rotn_dur;
    Serial.println("pin was high with value  ");
    Serial.print(val);
    for (int i = 20; i<rpm_entr; i++) {
      counter = i;
      ESC.write(counter);  // Send the signal to the ESC
      delay(200);
      Serial.println("current counter value: ");
      Serial.print(counter);
    }
    // put the code here to monitor speed and time remaining. After every 300 ms it must check if the desired amount of time has elapsed or not and stop if the time has completed.
    time1 = millis();  // the time at which the rotation started
    compare = 0;  // this line is necessary to perform multiple sets of rotations. else the below while loop will not be executed.
    time2 = 0;    // not that necessary. Once a garbage value will be displayed.
    while (compare <= rotn_dur) {
      compare = (time2 - time1) / 1000;  // time elapsed
      time_lft = rotn_dur - compare;     // time left for the current rotations to complete.
      h = time_lft;                      // time left for the current rotations to complete.
      Serial.println("Time remaining: ");
      Serial.print(time_lft);
      delay(200);
      time2 = millis();
    }
    for (int i = rpm_entr; i > 10; i--)  // i>10 is such that the speed of the motor will have been reduced to zero.
    {
      counter = i;
      ESC.write(counter);  // Send the signal to the ESC
      delay(200);
      Serial.print("counter ");
      Serial.println(counter);
      flag = LOW;
    }
  }
}
