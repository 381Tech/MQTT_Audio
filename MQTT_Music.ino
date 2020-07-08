// _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// DFPlayer Controll
// _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// ========================================
// 設定関連の関数を覚えてられないので以下メモ
/*
  myDFPlayer.next();                    //Play next mp3
  myDFPlayer.previous();                //Play previous mp3
  myDFPlayer.play(1);                   //Play the first mp3
  myDFPlayer.loop(1);                   //Loop the first mp3
  myDFPlayer.pause();                   //pause the mp3
  myDFPlayer.start();                   //start the mp3 from the pause
  myDFPlayer.playFolder(15, 4);         //play specific mp3 in SD:/15/004.mp3; Folder Name(1~99); File Name(1~255)
  myDFPlayer.enableLoopAll();           //loop all mp3 files.
  myDFPlayer.disableLoopAll();          //stop loop all mp3 files.
  myDFPlayer.playMp3Folder(4);          //play specific mp3 in SD:/MP3/0004.mp3; File Name(0~65535)
  myDFPlayer.advertise(3);              //advertise specific mp3 in SD:/ADVERT/0003.mp3; File Name(0~65535)
  myDFPlayer.stopAdvertise();           //stop advertise
  myDFPlayer.playLargeFolder(2, 999);   //play specific mp3 in SD:/02/004.mp3; Folder Name(1~10); File Name(1~1000)
  myDFPlayer.loopFolder(5);             //loop all mp3 files in folder SD:/05.
  myDFPlayer.randomAll();               //Random play all the mp3.
  myDFPlayer.enableLoop();              //enable loop.
  myDFPlayer.disableLoop();             //disable loop.
*/

// _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// MQTT_Music.ino
// _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// ========================================
// DFPlayerをシリアルで制御しつつ、brokerから拾った文字列によって再生・停止を行うプログラム(マイコンボードはESP32)
//  本当はデバッグ用のシリアルと制御用のシリアルをわける(前者はソフトウェアシリアル、後者はハードウェアシリアル)のが綺麗ですが…。
//  何故かハードウェアシリアルが動かないのでシリアルが共存してしまっているという吐き気を催す構成となっている。
//  created by 381Tech (2020.07.07)


#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"
DFRobotDFPlayerMini myDFPlayer;
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "CDebounce.h"

// _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// Global Definition
// _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// ========================================
// Wi-Fi setting
// 接続先のSSIDとパスワード
const char* wifi_ssid = "*********";
const char* wifi_password = "********";

// ========================================
// MQTT setting
// shiftr.io サイトの情報、Token は自分で作成した Channel から取得
const char* mqttServer   = "broker.shiftr.io";
const int   mqttPort     = 1883;
const char* mqttUser     = "********";
const char* mqttPassword = "********";

// ========================================
// MQTT setting
// 自分で設定するデバイス名
const char* mqttDeviceId = "AudioPlayer";

// MQTT Topic 名
const char* mqttTopic_publish   = "****";
const char* mqttTopic_subscribe = "****";

//Connect WiFi Client and MQTT(PubSub) Client
WiFiClient espClient;
PubSubClient g_mqtt_client(espClient);

// ========================================
// JSON setting
// Reserved memory for deserialization
//   enough space for 1 object with 10 members
const int json_object_capacity = 1 * JSON_OBJECT_SIZE(10);

// ========================================
// ESP32 pin configuration
// pin number definition
const int BUSY_STATE = 32;
const int R_SW = 34;
const int Y_SW = 35;
const int G_LED = 17;

int busy_flag = 0;

// _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// Status monitor for debug
// _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// ========================================
// Show status in loop function
//   This function is designed to be called in loop() function, 
//   to show system status regulary on serial console. 
void show_status_loop()
{
    const unsigned long print_interval = 5000; // [ms] of print interval
    static unsigned long last_print_time = millis(); // last time stamp of print

    // if elapsed time is too short...
    if((millis() - last_print_time) < print_interval){
        return; // quit doing nothing
    }

    // update timestamp
    last_print_time = millis();

    // show Wi-Fi status, if not connected
    if(WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi: not connected.");
    }

    // show MQTT status, if not connected
    if(!g_mqtt_client.connected()) {
        Serial.println("MQTT: not connected.");
    }

    return;
}

// _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// MQTT
// _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// ========================================
// Connect to MQTT broker
void connect_to_MQTT_broker()
{
    // Attempt to connect
    if (g_mqtt_client.connect(mqttDeviceId, mqttUser, mqttPassword)) {
        // subscribe here
        g_mqtt_client.subscribe(mqttTopic_subscribe);
        Serial.print("MQTT: Subscribed to topic: ");
        Serial.println(mqttTopic_subscribe);
    } 

    // Show info.
    if(g_mqtt_client.connected()){
        Serial.println("MQTT: Connected.");
    }
}

// ========================================
// MQTT callback on message arrival
//   Note! payload may not be null-terminated.
void MQTT_callback(char* topic, byte* payload, unsigned int length) 
{
	// print message
	Serial.print("MQTT: Received topic = "); 
	Serial.print(topic); 
	Serial.print(", payload = ");
	Serial.write(payload, length); // using write() here, since payload may not be null-terminated
    Serial.println();


    // Deserialize (or parse) JSON
     StaticJsonDocument<json_object_capacity> json_doc;
     DeserializationError err = deserializeJson( json_doc, (char*)payload, length );
     if(err){
         Serial.print(F("deserializeJson() failed with code "));
         Serial.println(err.c_str());
          return;
    }

    // // Find element in JSON
    String dudge = json_doc["STATUS"];
    Serial.println(dudge);
    if( dudge == "BELL" ){
        Serial.println("MUSIC START:HOT LIMIT");
        myDFPlayer.playMp3Folder(1); //TrackNo.1 Hotlimit
      }
    if( dudge == "BELL2" ){
        Serial.println("MUSIC START:ULTRA SOUL");
        myDFPlayer.playMp3Folder(2); //TrackNo.2 ultra soul
      }
    if( dudge == "BELL3" ){
        Serial.println("MUSIC START:ATSUMARE PARIPI!");
        myDFPlayer.playMp3Folder(3); //TrackNo.3 あつまれパーリーピーポー
      }
    if( dudge == "BELL4"){
        Serial.println("MUSIC START:ZEN ZEN ZEN SE");
        myDFPlayer.playMp3Folder(4); //TrackNo.4 前前々世
      }
    if ( dudge == "BELLSTART"){
        Serial.println("▼再度攻撃を仕掛けられた！");
        myDFPlayer.start();          //再生停止
      }
    if ( dudge == "BELLSTOP"){
        Serial.println("▼操作者から許された！");
        myDFPlayer.pause();          //再生停止
      }

}
/*
void receive_json_doc(){
    String dudge = json_doc["STATUS"];
    if( dudge == "BELL" ){
        Serial.println("YO SAY!");
        myDFPlayer.play(1);  //TrackNo.1 Hotlimit
        }else if( dudge == "BELL2"){
            Serial.println("Ultra Soul!");
            myDFPlayer.play(1);
            myDFPlayer.next();
            }else if ( dudge == "BELLSTOP"){
                Serial.println("ENDLESS SUMMER!");
                myDFPlayer.pause();  //pause the mp3
    }
}
*/

// ========================================
// Publish MQTT message 
void publish_signal_state(String state)
{
    StaticJsonDocument<json_object_capacity> json_doc;

    json_doc["deviceName"] = mqttDeviceId;
    json_doc["STATE"] = state;

    String payload;
    serializeJson( json_doc, payload );
    g_mqtt_client.publish(mqttTopic_publish, payload.c_str());

    // print message
    Serial.print("MQTT: Publish to ");
    Serial.print(mqttTopic_publish);
    Serial.print(", payload = ");
    Serial.println(payload);
}

// _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// Push Button
// _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// ========================================
// Push-button callback functions
void callback_sw2(){
    Serial.println("SW2 pushed.");
    publish_signal_state("WAKEUP");
    myDFPlayer.pause();
}

// ========================================
// Assign input pin to its callback funciton, with debouncing
CDebounce cSW2_deb(R_SW, callback_sw2, FALLING);

/* DFPlayer */
void DFP_setting(){
  
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  //if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
  if (!myDFPlayer.begin(Serial)) {  //Use HardwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true){
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
  Serial.println(F("DFPlayer Mini online."));
  myDFPlayer.volume(15);  //Set volume value. From 0 to 30
}

void AudioState(){
    if(digitalRead(BUSY_STATE) == HIGH){  //再生中かどうか状態監視(HIGH：停止中　LOW：再生中)
      busy_flag = 1;
    }else{
      busy_flag = 0;
    }
}

// _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// Arduino
// _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// ========================================
// Setup and loop
void setup()
{
    Serial.begin(9600);

    pinMode(BUSY,INPUT);        //BUSY監視用
    pinMode(R_SW,INPUT);        //RED Switch
    pinMode(Y_SW,INPUT);        //Yellow Switch
    pinMode(G_LED,OUTPUT);      //インジケータ用LED
    
    DFP_setting();              //Audio関連の設定関数

    // connect to WiFi
    Serial.print("WiFi: SSID: ");
    Serial.println(wifi_ssid);
    WiFi.begin(wifi_ssid, wifi_password);

    // set MQTT broker and callback
    Serial.print("MQTT: Broker: ");
    Serial.println(mqttServer);
    g_mqtt_client.setServer(mqttServer, mqttPort);
    g_mqtt_client.setCallback(MQTT_callback);

}

void loop()
{
    // MQTT broker task
    if(!g_mqtt_client.connected()){
        connect_to_MQTT_broker();
    } else {
        g_mqtt_client.loop();
    }

    // button check... event detection makes callback function call. 
    cSW2_deb.check();

    // show status on console
    show_status_loop();
}