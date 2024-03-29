#include "WifiConfig.h"    
#include "EEPROM.h"

#define LENGTH(x) (strlen(x) + 1)  
#define EEPROM_SIZE 200             
#define WIFI_RESET 0        

void setUpAPMode(); 
void getWifiConfigs();

void getConfigEEPROM();
void save_Wifi_EEPROM();

String read_flash(int startAddr);
void write_flash(const char* toStore, int startAddr);
void delete_flash();

// String extractURL(const char *url_text, const char *pattern1, const char *pattern2);
// char *extract(const char *const string, const char *const left, const char *const right);
String toExtract(const char *string, const char *left, const char *right);

unsigned long millis_RESET;

// Set these to your desired credentials.
const char* host = "smarttmfu";
const char *ssidAP = "SmartTMfu-WifiNode";
const char *passwordAP = "12345678";
/*
 * Login page
 */
const char* loginIndex =
"<style>"
        "#loginForm {"
            "width: 40%;"
            "margin: 0 auto;"
            "padding: 20px;"
            "background-color: #A0DAA9;"
            "border-radius: 10px;"
            "box-shadow: 0 0 10px rgba(0, 0, 0, 0.2);"
        "}"
        "#loginForm h2 {"
            "text-align: center;"
            "color: #006400;"
        "}"
        "#loginForm table {"
            "width: 90%;"
            "border: none;"
        "}"
        "#loginForm input[type='text'],"
        "#loginForm input[type='password'] {"
            "width: 100%;"
            "padding: 10px;"
            "margin: 5px 0;"
            "border: 1px solid #006400;"
            "border-radius: 5px;"
        "}"
        "#loginForm input[type='submit'] {"
            "width: 60%;"
            "padding: 10px;"
            "background-color: #006400;"
            "color: #FFFFFF;"
            "border: none;"
            "border-radius: 5px;"
            "cursor: pointer;"
        "}"
        "@media screen and (max-width: 768px) {"
            "#loginForm {"
                "width: 60%;"
            "}"
        "}"
    "</style>"
 "<form id='loginForm' name='loginForm'>"
    "<h2>SmartTmfu.com setting Wifi Page</h2>"
    "<table>"
        "<tr>"
             "<td>Wifi SSID:</td>"
             "<td><input type='text' name='userid'><br></td>"
        "</tr>"
        "<tr>"
            "<td>Password:</td>"
            "<td><input type='Password' name='pwd'><br></td>"
        "</tr>"
        "<tr>"
            "<td>Token key:</td>"
            "<td><input type='text' name='tokenkey'><br></td>"
        "</tr>"
        "<tr>"
            "<td colspan='2'><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Push Close to confirm')/*displays error message*/"
    "}"
    "}"
"</script>";
 
WiFiServer server(80);

String userid = "";
String pwd = "";
String token = ""; 
int index_check = 0;

int wifiConnected = 0;
int getWifiConfig = 0;

/* ----------------------------------------- Get Wifi Flow ----------------------------------------- 
// 1. Get Config from EPPROM first
// 2. If can not connect using this config -> turn on Wifi AP Mode to set up web server to get new Config
// Both LED turn on while webserver is on
*/
void WifiConfig() {
  /* ------------------ Get EEPROM ------------------ */  
  getConfigEEPROM(); // checking for 10 seconds
  
  /* Use this if you want to delete wifi config manually */  
  // delete_flash();

  // If can not connect to Wifi 
  if (WiFi.status() != WL_CONNECTED){
    /* ------------------ Get to STA Mode ------------------ */ 
    setUpAPMode();
    millis_RESET = millis(); 
    while(wifiConnected != 1){ 
      // Get wifi configs from web server on ESP     
      digitalWrite(2, HIGH);  
      // digitalWrite(13, HIGH);  
      delay(400);              
      while(getWifiConfig == 0){
        getWifiConfigs(); // return getWifiConfig = 1 after get Config from webserver
        // Reset if take too long, 180s
        if (millis() - millis_RESET >= 180000){  // 3 mins
          Serial.println("\n\n\n\n\n\nRestarting the ESP"); 
          ESP.restart();            
        } 
      }  
      digitalWrite(2, LOW);   
      // digitalWrite(13, LOW);   
      delay(1000);  
  
      // Set Wifi to STA mode 
      Serial.println("WIFI mode : STA");
      WiFi.mode(WIFI_STA);  
      Serial.print("Connecting to ");
      Serial.println(userid); 
      WiFi.begin(userid, pwd); 
      
      // Try to connect for 20s
      for (int i=0; i<30; i++) { 
        Serial.print(".");
        delay(1000); 
        if(WiFi.status() == WL_CONNECTED){ 
          Serial.println("\nWiFi connected");
          Serial.println("IP address: ");
          Serial.println(WiFi.localIP());
          // Save Wifi to eeprom
          save_Wifi_EEPROM();
          return;
        }
      }  
      // If Wifi not connected, Config is incorrect, keep run  
      if (WiFi.status() != WL_CONNECTED) { 
        setUpAPMode();
        getWifiConfig = 0; // open webserver again
      } 	                
    }
  }  
}

// ===================================================================================================
/* Get config from EEPROM*/
void getConfigEEPROM(){
  // Setup epprom
  pinMode(WIFI_RESET, INPUT); 
  Serial.println("----------------- getConfigEEPROM ----------------");
  if (!EEPROM.begin(EEPROM_SIZE)) { 
    Serial.println("Failed to init EEPROM");
    delay(1000);
  }
  else{
    userid = read_flash(0); 
    Serial.print("SSID = ");
    Serial.println(userid);
    pwd = read_flash(40); 
    Serial.print("Password = ");
    Serial.println(pwd);
    token = read_flash(80); 
    Serial.print("Token = ");
    Serial.println(token);
  }
  WiFi.mode(WIFI_STA);  
  WiFi.begin(userid.c_str(), pwd.c_str());
  Serial.println("Connecting to: "+userid+"| Password: "+pwd+"| Token: "+token);  
  // Try to connect for 30s
  for (int i=0; i<30; i++) { 
    delay(1000);  
    Serial.print(".");
    if(WiFi.status() == WL_CONNECTED){
      Serial.println("");
      return;
    }
  }  
}

void setUpAPMode(){
  /* ------------------------------------------ Get to AP Mode ------------------------------------------ */
  delay(10);
  WiFi.mode(WIFI_AP);
  Serial.println("Configuring access point...");  
  // You can remove the password parameter if you want the AP to be open. a valid password must have more than 7 characters
  if (!WiFi.softAP(ssidAP, passwordAP)) {
    log_e("Soft AP creation failed.");
    while(1);
  }
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();
  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started"); 
  Serial.println("Server started");      
}

//get config from esp32 web
void getWifiConfigs(){ 
  /* ------------------------------------------ Get Configs ------------------------------------------ */
  WiFiClient client = server.available();   // listen for incoming clients 
  if (client) {                             // if you get a client,
    Serial.println("\n\n------------------------");     
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client

    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        // Serial.println("c: ");    
        Serial.write(c);                    // print it out the serial monitor
        
        if (c == '\n') {                    // if the byte is a newline character 
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            //server.send(200, "text/html", loginIndex);
              
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.println(loginIndex);
            // the content of the HTTP response follows the header:
            /*
            client.print("Click <a href=\"/H\">here</a> to turn ON the LED.<br>");
            client.print("Click <a href=\"/L\">here</a> to turn OFF the LED.<br>");

            // The HTTP response ends with another blank line:
            client.println();
              */            
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } 
        else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        } 
        if (currentLine.startsWith("GET /?userid")  && currentLine.endsWith("HTTP/1.1")) { 
          // Reset Config  
          index_check = 0;
          // Extract Configs     
          // for (int i = 0; i < currentLine.length(); i++) { 
          //   if (currentLine[i] == '&') {   
          //     index_check++;     
          //   } 
          //   if (index_check==1){
          //     userid += currentLine[i];     
          //   }
          //   if (index_check==3){
          //     pwd += currentLine[i];     
          //   }
          //   if (index_check==5){
          //     token += currentLine[i];      
          //     if (currentLine[i] == ' ') {   
          //       index_check++;     
          //     }
          //   }
          //   if (currentLine[i] == '=') {   
          //     index_check++;     
          //   }
          // }      
          // close the connection:
          userid = "";
          pwd = "";
          token = "";    
          
          // char currentLineArray[] = currentLine;
          // MSG: GET /?userid=abc&pwd=abc&tokenkey=abc HTTP/1.1
          userid = toExtract(currentLine.c_str(), "userid=", "&pwd");
          pwd = toExtract(currentLine.c_str(), "pwd=", "&tokenkey");
          token = toExtract(currentLine.c_str(), "tokenkey=", " HTTP/1.1");
          
          Serial.println("\n--------------- Config from web --------------");
          Serial.println("UserID: "+userid);
          Serial.println("pwd: "+pwd);
          Serial.println("token: "+token);     
          getWifiConfig = 1;
          return;        
        }
      }
    }
            
    client.stop();
    Serial.println("Client Disconnected.");    
  } 
}

// Extract
String toExtract(const char *string, const  char *left, const char *right){
    char  *head;
    char  *tail;
    size_t length;
    char  *result;
    if ((string == NULL) || (left == NULL) || (right == NULL))
        return "";
    length = strlen(left);
    head   = strstr(string, left);
    if (head == NULL)
        return "";
    head += length;
    tail  = strstr(head, right);
    if (tail == NULL)
        return String(tail);
    length = tail - head;
    result = (char*)malloc(1 + length);
    if (result == NULL)
        return "";
    result[length] = '\0';
    memcpy(result, head, length);
    
    String extracted = String(result);    
    return extracted;
}

// char *extract(const char *const string, const char *const left, const char *const right){
//     char  *head;
//     char  *tail;
//     size_t length;
//     char  *result;
//     if ((string == NULL) || (left == NULL) || (right == NULL))
//         return NULL;
//     length = strlen(left);
//     head   = strstr(string, left);
//     if (head == NULL)
//         return NULL;
//     head += length;
//     tail  = strstr(head, right);
//     if (tail == NULL)
//         return tail;
//     length = tail - head;
//     result = (char*)malloc(1 + length);
//     if (result == NULL)
//         return NULL;
//     result[length] = '\0';
//     memcpy(result, head, length);    
//     return result;
// }

// String extractURL(const char *url_text, const char *pattern1, const char *pattern2){
//   // const char *s = "aaaaaa<BBBB>TEXT TO EXTRACT</BBBB>aaaaaaaaa";
//   // const char *PATTERN1 = "<BBBB>";
//   // const char *PATTERN2 = "</BBBB>";

//   char *target = NULL;
//   char *start, *end;

//   if (start = strstr(url_text, pattern1)){
//     start += strlen( pattern1 );
//     if (end = strstr(start, pattern2)){
//         target = ( char * )malloc( end - start + 1 );
//         memcpy(target, start, end - start);
//         target[end - start] = '\0';
//     }
//   }
//   if(target) printf("%s\n", target);


//   String extracted = "";
//   strcpy(extracted, target);
//   free(target);
//   return extracted;
// }


// ------------------------------------------------- EEPROM ------------------------------------------------- 
void save_Wifi_EEPROM(){
  userid = WiFi.SSID();
  pwd = WiFi.psk();
  Serial.print("userid:");
  Serial.println(userid);
  Serial.print("pwd:");
  Serial.println(pwd);
  Serial.print("token:");
  Serial.println(token);
  Serial.println("Store Config in Flash");
  write_flash(userid.c_str(), 0); 
  write_flash(pwd.c_str(), 40); 
  write_flash(token.c_str(), 80); 
}
 
void write_flash(const char* toStore, int startAddr) {
  int i = 0;
  for (; i < LENGTH(toStore); i++) {
    EEPROM.write(startAddr + i, toStore[i]);
  }
  EEPROM.write(startAddr + i, '\0');
  EEPROM.commit();
}

String read_flash(int startAddr) {
  char in[128]; 
  int i = 0;
  for (; i < 128; i++) {
    in[i] = EEPROM.read(startAddr + i);
  }
  return String(in);
}

void delete_flash(){
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  delay(500);    
}

String getTokenKey(){
  String myTokenKey = read_flash(80);
  return myTokenKey;
}