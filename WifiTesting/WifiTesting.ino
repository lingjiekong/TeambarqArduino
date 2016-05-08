//void setup() {
//  pinMode(0, OUTPUT);
//}
//
//void loop() {
//  digitalWrite(0, HIGH);
//  delay(500);
//  digitalWrite(0, LOW);
//  delay(500);
//}


//// Mac Address 5c:cf:7f:00:6c:6c:
//#include <ESP8266WiFi.h>
//
//uint8_t MAC_array[6];
//char MAC_char[18];
//
//void setup() {
//    Serial.begin(115200);
//    Serial.println();
//
//    WiFi.macAddress(MAC_array);
//    for (int i = 0; i < sizeof(MAC_array); ++i){
//      sprintf(MAC_char,"%s%02x:",MAC_char,MAC_array[i]);
//    }
//  
//    Serial.println(MAC_char);
//}
//
//void loop() { 
//}


      /*
 *  Simple HTTP get webclient test
 */
#include <ESP8266WiFi.h>

const char* ssid     = "Stanford";
const char* password = "";

const char* host = "www.adafruit.com";

void setup() {
  Serial.begin(115200);
  delay(100);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

int value = 0;

void loop() {
  delay(5000);
  ++value;

  Serial.print("connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  // We now create a URI for the request
  String url = "/testwifi/index.html";
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(500);
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection");
}


