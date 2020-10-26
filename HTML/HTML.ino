/*****************************
 * 设计：彭昕
 * 所用字体：u8g2_font_freedoomr10_mu
 * ***************************/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include "WeatherStationImages.h"
//U8G2 初始化头文件//
#include <Arduino.h>
#include <U8g2lib.h>
#include <Time.h>
#include <TimeLib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
String          tim   = "00:00";
String          dat   = "2019/01/01";
#define model_change 6 //配置变更的时间
IPAddress timeServer(139,199,215,251);                         /* 阿里云ntp服务器 如果失效可以使用 120.25.115.19   120.25.115.20 */
#define STD_TIMEZONE_OFFSET +8                                     /* 设置中国 */
const int timeZone = 8;                                            /* 修改北京时区 */
WiFiUDP   Udp;
unsigned int  localPort = 8266;
//需要打开WEB_OTA
#define WEB_OTA
// #define ESP8266_01
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R2, /* clock=*/ 0, /* data=*/ 2, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display
//U8G2 初始化头文件//
// wifi 参数
const char* ssid = "ASUS";
const char* password = "887385206";
const char* host = "192.168.2.167"; //需要访问的域名
const int httpsPort = 8080;  // 需要访问的端口
const String url = "/sse";   // 需要访问的地址
HTTPClient http;
WiFiClient client;
//全局变量保存解析的数据
int CPU_FREQ;
int GPU_FREQ;
int CPU_TEMP;
int GPU_TEMP;
int CPU_FAN;
int GPU_FAN;
int CASE_FAN;
int GPU_USE;
int CPU_USE;
int MODEL;
int SWITCH_TIME;   
int value = 0;
int flow_flag;
unsigned long last_time;
unsigned long http_success_time;
unsigned long http_fail_time;
#ifdef WEB_OTA
ESP8266WebServer webServer(80);
ESP8266HTTPUpdateServer httpUpdater;
#endif
void setup() {
// 启动串口
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  delay(2000);
  while (WiFi.status() != WL_CONNECTED) //连接WiFi
  {
    delay(500);
    Serial.print(".");
    u8g2_init();
    delay(2000);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());//打印出开发板的IP地址
  Udp.begin(localPort);
  #ifdef ESP8266_01
  pinMode(1, FUNCTION_3); 
  //GPIO 3 (RX) swap the pin to a GPIO.
  pinMode(3, FUNCTION_3);
  #endif 
  #ifdef WEB_OTA
  httpUpdater.setup(&webServer); //httpUpdater绑定到webServer上
  webServer.begin(); //启用WebServer
  #endif
}

void loop() {
  sendrequest();
  if((http_fail_time > http_success_time) & (http_fail_time - http_success_time) >= (model_change * 1000))
  {
    get_time();
    draw_TIME();
    #ifdef WEB_OTA
    webServer.handleClient(); //处理http事务
    #endif
  }else{
    if(MODEL == 0)
      draw_GPU();
    else if(MODEL == 1)
      draw_CPU();
    else{
      if(flow_flag == 0){
        draw_CPU();
      }else{
        draw_GPU(); 
      }
      if(millis()-last_time >=(SWITCH_TIME * 1000)){
        flow_flag = ~flow_flag;
        last_time = millis();
      }
    }
  }
}


void getPrice(String s) {
  int datStart = 0, datEnd = 0;
  String datstr;
 
  char cpu_freq[] = "CPU_FREQ";
  datStart = s.indexOf(cpu_freq) + strlen(cpu_freq);
  datEnd = s.indexOf("MHz", datStart);
  datstr = s.substring(datStart, datEnd);
  CPU_FREQ = datstr.toInt();
 
 
  char gpu_freq[] = "GPU_FREQ";
  datStart = s.indexOf(gpu_freq) + strlen(gpu_freq);
  datEnd = s.indexOf("MHz", datStart);
  datstr = s.substring(datStart, datEnd);
  GPU_FREQ = datstr.toInt();


  char cpu_temp[] = "CPU_TEMP";
  datStart = s.indexOf(cpu_temp) + strlen(cpu_temp);
  datEnd = s.indexOf("℃", datStart);
  datstr = s.substring(datStart, datEnd);
  CPU_TEMP = datstr.toInt();


  char gpu_temp[] = "GPU_TEMP";
  datStart = s.indexOf(gpu_temp) + strlen(gpu_temp);
  datEnd = s.indexOf("℃", datStart);
  datstr = s.substring(datStart, datEnd);
  GPU_TEMP = datstr.toInt();

  char cpu_fan[] = "CPU_FAN";
  datStart = s.indexOf(cpu_fan) + strlen(cpu_fan);
  datEnd = s.indexOf("RPM", datStart);
  datstr = s.substring(datStart, datEnd);
  CPU_FAN = datstr.toInt();


  char gpu_fan[] = "GPU_FAN";
  datStart = s.indexOf(gpu_fan) + strlen(gpu_fan);
  datEnd = s.indexOf("RPM", datStart);
  datstr = s.substring(datStart, datEnd);
  GPU_FAN = datstr.toInt();

  char case_fan[] = "CASE_FAN";
  datStart = s.indexOf(case_fan) + strlen(case_fan);
  datEnd = s.indexOf("RPM", datStart);
  datstr = s.substring(datStart, datEnd);
  CASE_FAN = datstr.toInt();
  
  char cpu_use[] = "CPU_USE";
  datStart = s.indexOf(cpu_use) + strlen(cpu_use);
  datEnd = s.indexOf("%", datStart);
  datstr = s.substring(datStart, datEnd);
  CPU_USE = datstr.toInt();

  char gpu_use[] = "GPU_USE";
  datStart = s.indexOf(gpu_use) + strlen(gpu_use);
  datEnd = s.indexOf("%", datStart);
  datstr = s.substring(datStart, datEnd);
  GPU_USE = datstr.toInt();

  char model[] = "SET";
  datStart = s.indexOf(model) + strlen(model);
  datEnd = s.indexOf("&", datStart);
  datstr = s.substring(datStart, datEnd);
  MODEL = datstr.toInt();

  char set_time[] = "&";
  datStart = s.indexOf(set_time) + strlen(set_time);
  datEnd = s.indexOf("!", datStart);
  datstr = s.substring(datStart, datEnd);
  SWITCH_TIME = datstr.toInt();

  Serial.println(MODEL);
  Serial.println(SWITCH_TIME);
//  //Serial.println(s);
//  Serial.print("CPU_FREQ");
//  Serial.println(CPU_FREQ);
//  Serial.print("GPU_FREQ");
//  Serial.println(GPU_FREQ);
//  Serial.print("CPU_TEMP");
//  Serial.println(CPU_TEMP);
//  Serial.print("GPU_TEMP");
//  Serial.println(GPU_TEMP);
//  Serial.print("CPU_FAN");
//  Serial.println(CPU_FAN);
//  Serial.print("GPU_FAN");
//  Serial.println(GPU_FAN);
//  Serial.print("CASE_FAN");
//  Serial.println(CASE_FAN);
  
}

void sendrequest()
{
    WiFiClient client;  

  /**
    * 测试是否正常连接
    */
  if (!client.connect(host, httpsPort)) {  
     Serial.println("connection failed");
     http_fail_time = millis();  
    return;  
   }  
  delay(10);   
  String postRequest =(String)("GET ") + url + " HTTP/1.1\r\n" +  
    "Content-Type: text/html;charset=utf-8\r\n" +  
     "Host: " + host + "\r\n" + 
    "User-Agent: BuildFailureDetectorESP8266\r\n" +
     "Connection: close\r\n\r\n";   
  client.print(postRequest);  // 发送HTTP请求
  client.print(postRequest);  // 发送HTTP请求
   /**
    * 展示返回的所有信息
   */
  char buffer[497];
  int numdata = 0;
  delay(100);
  numdata = client.readBytes(buffer,498);
  String temp = String(buffer);
  Serial.println(temp);//打印返回的信息
  getPrice(temp);//
  http_success_time = millis();
  delay(500);
}

void u8g2_prepare(void) 
{
  u8g2.setFont(u8g2_font_freedoomr10_mu);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void draw_GPU(void)
{
  u8g2_prepare();
  u8g2.clearBuffer();
  u8g2.drawXBMP(0, 0,gpulogow,gpulogoh, gpulogo);
  u8g2.setFont(u8g2_font_freedoomr10_mu);
  u8g2.setCursor(78, 0);
  u8g2.print("GPU"); 
  u8g2.setCursor(54, 16);
  u8g2.print("TEM:");
  u8g2.print(GPU_TEMP);
  u8g2.print("C");
  u8g2.setCursor(54, 28);
  u8g2.print("FAN:");
  u8g2.print(GPU_FAN);
  u8g2.setCursor(54, 40);
  u8g2.print("FRE:");
  u8g2.print(GPU_FREQ);
  u8g2.setCursor(54, 52);
  u8g2.print("USE:");
  u8g2.print(GPU_USE);
  u8g2.setFont(u8g2_font_t0_16_tn);
  u8g2.print("%");
  u8g2.setFont(u8g2_font_pcsenior_8u);//6PIXEL  
  u8g2.setCursor(10, 45);
  u8g2.print("RTX");
  u8g2.setCursor(6, 55);
  u8g2.print("2060");
  u8g2.sendBuffer();
}

void draw_CPU(void)
{
  u8g2_prepare();
  u8g2.clearBuffer();
  u8g2.drawXBMP(0, 0,cpulogow,cpulogoh, cpulogo); 
  u8g2.setFont(u8g2_font_freedoomr10_mu);
  u8g2.setCursor(78, 0);
  u8g2.print("CPU"); 
  u8g2.setCursor(56, 16);
  u8g2.print("TEM:");
  u8g2.print(CPU_TEMP);
  u8g2.print("C");
  u8g2.setCursor(56, 28);
  u8g2.print("FAN:");
  u8g2.print(CPU_FAN);
  u8g2.setCursor(56, 40);
  u8g2.print("FRE:");
  u8g2.print(CPU_FREQ);
  u8g2.setCursor(56, 52);
  u8g2.print("USE:");
  u8g2.print(CPU_USE);
  u8g2.setFont(u8g2_font_t0_16_tn);
  u8g2.print("%");
  u8g2.setFont(u8g2_font_pcsenior_8u);//6PIXEL
  u8g2.setCursor(15, 43);
  u8g2.print("I7");
  u8g2.setCursor(0, 53);
  u8g2.print("10875H");
  u8g2.sendBuffer();
}

void u8g2_init()
{
  u8g2.begin();
  u8g2_prepare();
  // u8g2.drawXBMP(32, 0, 64,64, manlogo);//Fallout点赞
  u8g2.drawStr(0,0,"WIFI CONNECTED:");
  u8g2.drawStr(0,15,"ASUS");
  u8g2.drawStr(0,30,"IP ADDRESS:");
  u8g2.setCursor(0,45);
  u8g2.print(WiFi.localIP());
  u8g2.sendBuffer();
//  u8g2.enableUTF8Print();
//  u8g2.setFont(u8g2_font_unifont_t_chinese2);
//  u8g2.setCursor( 0, 15 );
//  u8g2.print("彭昕");
//  u8g2.sendBuffer();
//  delay(1000);
}

const int NTP_PACKET_SIZE = 48;
byte    packetBuffer[NTP_PACKET_SIZE];

time_t getNtpTime()
{
  while ( Udp.parsePacket() > 0 );
  Serial.println( "连接时间服务器" );
  sendNTPpacket( timeServer );
  uint32_t beginWait = millis();
  while ( millis() - beginWait < 1500 )
  {
    int size = Udp.parsePacket();
    if ( size >= NTP_PACKET_SIZE )
    {
      Serial.println( "时间服务器应答" );
      Udp.read( packetBuffer, NTP_PACKET_SIZE );
      unsigned long secsSince1900;
      /* convert four bytes starting at location 40 to a long integer */
      secsSince1900 = (unsigned long) packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long) packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long) packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long) packetBuffer[43];
      return(secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
    }
  }
  Serial.println( "No NTP Response :-(" );
  return(0);
}


void sendNTPpacket( IPAddress &address )
{
  memset( packetBuffer, 0, NTP_PACKET_SIZE );

  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[3] = 0xEC;

  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  Udp.beginPacket( address, 123 );
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void get_time(){
  setSyncProvider(getNtpTime);
  /* setSyncProvider(getNtpTime); */
  String zov = "";
  if ( hour() < 10 )
  {
    zov = "0";
  }
  if ( minute() < 10 )
  {
    tim = zov + String( hour() ) + ":0" + String( minute() );
  }else{ tim = zov + String( hour() ) + ":" + String( minute() ); }
  dat = String( year() ) + "/" + String( month() ) + "/" + String( day() );
  Serial.print( tim );    /* 输出当前网络分钟 */
  Serial.print( dat );    /* 输出当前日期 */
}
//u8g2_font_inr19_mn 
void draw_TIME()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_freedoomr25_tn );//27pixel
  u8g2.setCursor(20, 10);
  u8g2.print(tim);
  u8g2.setFont(u8g2_font_freedoomr10_mu);
  u8g2.setCursor(25, 50);
  u8g2.print(dat);
  u8g2.sendBuffer();  
}
