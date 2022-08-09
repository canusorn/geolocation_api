#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "TimeLib.h"
#include <ArduinoJson.h>

#define ON_Board_LED 2
#define R D1
#define G D2
#define B D3

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
struct timelib_tm tinfo; //--> Structure that holds human readable time information;

const char *ssid = "noosi159";
const char *password = "12345678";

timelib_t now, initialt;
unsigned long previousMillisGetTimeDate = 0; //--> will store last time was updated
const long intervalGetTimeDate = 1000;       //--> The interval for updating the time and date
//----------------------------------------

//----------------------------------------Declaration of time and date variables
// Week Days
String weekDays[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Month names
String months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

unsigned long epochTime;
int currentHour, currentMinute, currentSecond;
String weekDay;
int monthDay, currentMonth;
String currentMonthName;
int currentYear, currentYearforSet;
String _DateTime;
//----------------------------------------

//----------------------------------------Variable declarations to hold weather data
String current_weather_Description;
String current_weather;
int current_temperature, current_pressure, current_humidity, current_wind_deg, current_visibility;
float current_wind_speed, current_feels_like, current_uv, current_dew_point;
String current_weather_sym;

String forecast_weather[4];
String forecast_weather_sym[4];
float forecast_temp_min[4];
float forecast_temp_max[4];
//----------------------------------------
// If using a city name : String current_serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&units=" + units + "&APPID=" + openWeatherMapApiKey;
// If using a city ID : String current_serverPath = "http://api.openweathermap.org/data/2.5/weather?id=" + city + "&units=" + units + "&APPID=" + openWeatherMapApiKey;
//----------------------------------------openweathermap API configuration
String openWeatherMapApiKey = "8f1fbcd3c4f818ddc07b09bba424e3fe";

String city = "Chiang Mai";
String countryCode = "TH";
String units = "metric";

String googleKey = "AIzaSyA-kDl8N3GZwcSD1ASdDoRSpoJi-ptREiY";
//----------------------------------------

//----------------------------------------Variable declaration for the json data and defining the ArduinoJson(DynamicJsonBuffer)
String strjsonBuffer;
String strjsonBufferCF;
DynamicJsonBuffer jsonBuffer;
//----------------------------------------

int timezone_offset;

void setup()
{
  Serial.begin(115200);
  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");
  Serial.println("connected...)");
  delay(500);
  pinMode(ON_Board_LED, OUTPUT);    //--> On Board LED port Direction output
  digitalWrite(ON_Board_LED, HIGH); //--> Turn off Led On Board

  timeClient.begin(); //--> Initialize a NTPClient to get time
  Get_location();
  Get_Weather_Data();
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);
}
void loop()
{
  //__________________________________________________________________________________________________Millis to display time & date
  unsigned long currentMillisGetTimeDate = millis();
  if (currentMillisGetTimeDate - previousMillisGetTimeDate >= intervalGetTimeDate)
  {
    previousMillisGetTimeDate = currentMillisGetTimeDate;

    now = timelib_get(); //--> Get the timestamp from the library

    timelib_break(now, &tinfo); //--> Convert to human readable format

    //----------------------------------------Processing time and date data
    char timeProcess[8];
    sprintf(timeProcess, "%02u:%02u:%02u", tinfo.tm_hour, tinfo.tm_min, tinfo.tm_sec);
    char _time[8];
    strcpy(_time, timeProcess);

    char dateDayProcess[2];
    sprintf(dateDayProcess, "%02u", tinfo.tm_mday);
    char _dateDay[2];
    strcpy(_dateDay, dateDayProcess);

    _DateTime = _dateDay;
    _DateTime += "-" + currentMonthName + "-" + String(currentYear) + " " + _time;

    //----------------------------------------
  }
}
//=====================================================================================

//=====================================================================================Subroutines for getting weather data from openweathermap
void Get_Weather_Data()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    //__________________________________________________________________________________________________Get longitude, latitude and timezone offset
    //----------------------------------------If using a city name
    String current_serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&units=" + units + "&APPID=" + openWeatherMapApiKey;
    //----------------------------------------

    //----------------------------------------If using a city ID
    // String current_serverPath = "http://api.openweathermap.org/data/2.5/weather?id=" + city + "&units=" + units + "&APPID=" + openWeatherMapApiKey;
    //----------------------------------------

    Serial.println("--------------------");
    Serial.println("Get longitude, latitude & timezone offset from openweathermap...");
    Serial.println();
    strjsonBuffer = httpGETRequest(current_serverPath.c_str());
    // Serial.println(strjsonBuffer);
    JsonObject &root = jsonBuffer.parseObject(strjsonBuffer);

    // Test if parsing succeeds.
    if (!root.success())
    {
      Serial.println("parseObject() failed");
      delay(500);
      return;
    }

    String lon = root["coord"]["lon"];
    String lat = root["coord"]["lat"];
    timezone_offset = root["timezone"];

    Serial.println();
    Serial.print("Lon : ");
    Serial.println(lon);
    Serial.print("Lat : ");
    Serial.println(lat);
    Serial.print("Timezone offset : ");
    Serial.println(timezone_offset);
    Serial.println();

    jsonBuffer.clear();

    Serial.println("Set the Date and Time from the Internet...");
    Set_Time_and_Date();
    //__________________________________________________________________________________________________

    //__________________________________________________________________________________________________Get current weather data and weather forecast data for the following days
    //----------------------------------------Get current weather data
    String current_forecast_serverPath = "http://api.openweathermap.org/data/2.5/onecall?lat=" + lat + "&lon=" + lon + "&units=" + units + "&exclude=hourly&APPID=" + openWeatherMapApiKey;

    Serial.println("Get weather data from openweathermap...");
    strjsonBufferCF = httpGETRequest(current_forecast_serverPath.c_str());
    // Serial.println(strjsonBuffer);
    JsonObject &rootCF = jsonBuffer.parseObject(strjsonBufferCF);

    // Test if parsing succeeds.
    if (!rootCF.success())
    {
      Serial.println("parseObject() failed");
      delay(500);
      return;
    }

    String str_current_weather = rootCF["current"]["weather"][0]["main"];
    current_weather = str_current_weather;

    String str_current_weather_Description = rootCF["current"]["weather"][0]["description"];
    current_weather_Description = str_current_weather_Description;

    String str_current_weather_sym = rootCF["current"]["weather"][0]["icon"];
    current_weather_sym = str_current_weather_sym;

    current_temperature = rootCF["current"]["temp"];
    current_feels_like = rootCF["current"]["feels_like"];
    current_uv = rootCF["current"]["uvi"];
    current_dew_point = rootCF["current"]["dew_point"];
    current_pressure = rootCF["current"]["pressure"];
    current_humidity = rootCF["current"]["humidity"];
    current_visibility = rootCF["current"]["visibility"];
    current_wind_speed = rootCF["current"]["wind_speed"];
    current_wind_deg = rootCF["current"]["wind_deg"];
    //----------------------------------------

    //----------------------------------------Get weather forecast data for the following days
    String str_forecast_weather1 = rootCF["daily"][0]["weather"][0]["main"];
    forecast_weather[0] = str_forecast_weather1;
    String str_forecast_weather_sym1 = rootCF["daily"][0]["weather"][0]["icon"];
    forecast_weather_sym[0] = str_forecast_weather_sym1;
    forecast_temp_max[0] = rootCF["daily"][0]["temp"]["max"];
    forecast_temp_min[0] = rootCF["daily"][0]["temp"]["min"];

    String str_forecast_weather2 = rootCF["daily"][1]["weather"][0]["main"];
    forecast_weather[1] = str_forecast_weather2;
    String str_forecast_weather_sym2 = rootCF["daily"][1]["weather"][0]["icon"];
    forecast_weather_sym[1] = str_forecast_weather_sym2;
    forecast_temp_max[1] = rootCF["daily"][1]["temp"]["max"];
    forecast_temp_min[1] = rootCF["daily"][1]["temp"]["min"];

    String str_forecast_weather3 = rootCF["daily"][2]["weather"][0]["main"];
    forecast_weather[2] = str_forecast_weather3;
    String str_forecast_weather_sym3 = rootCF["daily"][2]["weather"][0]["icon"];
    forecast_weather_sym[2] = str_forecast_weather_sym3;
    forecast_temp_max[2] = rootCF["daily"][2]["temp"]["max"];
    forecast_temp_min[2] = rootCF["daily"][2]["temp"]["min"];

    String str_forecast_weather4 = rootCF["daily"][3]["weather"][0]["main"];
    forecast_weather[3] = str_forecast_weather4;
    String str_forecast_weather_sym4 = rootCF["daily"][3]["weather"][0]["icon"];
    forecast_weather_sym[3] = str_forecast_weather_sym4;
    forecast_temp_max[3] = rootCF["daily"][3]["temp"]["max"];
    forecast_temp_min[3] = rootCF["daily"][3]["temp"]["min"];
    //----------------------------------------
    Serial.println(" ");
    if (current_humidity >= 70)
    {
      Serial.println("RED");
      analogWrite(R, 255);
      analogWrite(G, 0);
      analogWrite(B, 0);
    }
    else if (current_humidity >= 21)
    {
      Serial.println("YELLOW");
      analogWrite(R, 255);
      analogWrite(G, 255);
      analogWrite(B, 0);
    }
    else if (current_humidity == 0)
    {
      analogWrite(R, 0);
      analogWrite(G, 0);
      analogWrite(B, 0);
    }
    else
    {
      Serial.println("GREEN");
      analogWrite(R, 0);
      analogWrite(G, 255);
      analogWrite(B, 0);
    }
    Serial.println();
    Serial.println("Current weather data");
    Serial.print("weather : ");
    Serial.println(current_weather);
    Serial.print("weather description : ");
    Serial.println(current_weather_Description);
    Serial.print("weather symbol : ");
    Serial.println(current_weather_sym);
    Serial.print("temperature : ");
    Serial.print(current_temperature);
    Serial.println(" °C");

    Serial.print("feels_like : ");
    Serial.print(current_feels_like);
    Serial.print(" °C");
    Serial.print(" (");
    Serial.print(round(current_feels_like));
    Serial.print(" °C");
    Serial.println(")");

    Serial.print("UV : ");
    Serial.print(current_uv);
    Serial.print(" (");
    Serial.print(round(current_uv));
    Serial.println(")");

    Serial.print("Dew point : ");
    Serial.print(current_dew_point);
    Serial.print(" °C");
    Serial.print(" (");
    Serial.print(round(current_dew_point));
    Serial.print(" °C");
    Serial.println(")");

    Serial.print("pressure : ");
    Serial.print(current_pressure);
    Serial.println(" hPa");

    Serial.print("humidity : ");
    Serial.print(current_humidity);
    Serial.println(" %");

    Serial.print("visibility : ");
    Serial.print(current_visibility);
    Serial.print(" m");
    Serial.print(" (");
    float current_visibility_to_km = current_visibility / 1000;
    Serial.print(current_visibility_to_km);
    Serial.print(" km");
    Serial.println(")");

    Serial.print("wind_speed : ");
    Serial.print(current_wind_speed);
    Serial.println(" m/s");

    Serial.print("wind_deg : ");
    Serial.print(current_wind_deg);
    Serial.println("°");
    //----------------------------------------

    //----------------------------------------

    jsonBuffer.clear();

    Serial.println("Set the Date and Time again from the Internet...");
    Set_Time_and_Date();
    //__________________________________________________________________________________________________
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
}
//=====================================================================================

//=====================================================================================Get Coordinate from google geolocationApi
void Get_location()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;

    String url = "https://www.googleapis.com/geolocation/v1/geolocate?key=" + googleKey;
    String json = "{\"wifiAccessPoints\": [{\"macAddress\": \"F4:09:D8:B7:46:25\",\"signalStrength\": -54,\"signalToNoiseRatio\": 0},{\"macAddress\": \"74:DA:38:DB:E7:C0\",\"signalStrength\": -43,\"signalToNoiseRatio\": 0}]}";

    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, url))
    { // HTTPS

      Serial.print("[HTTPS] POST...\n");
      https.addHeader("Content-Type", "application/json");
      // start connection and send HTTP header
      int httpCode = https.POST(json);

      // httpCode will be negative on error
      if (httpCode > 0)
      {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK)
        {
          String payload = https.getString();
          Serial.println(payload);
        }
      }
      else
      {
        Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    }
    else
    {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }
}
//=====================================================================================

//=====================================================================================httpGETRequest
String httpGETRequest(const char *serverName)
{
  HTTPClient http;

  // Your IP address with path or Domain name with URL path
  http.begin(serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode == 200)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    delay(500);
  }
  // Free resources
  http.end();

  return payload;
}
//=====================================================================================

//=====================================================================================Subroutines for setting the time and date from the internet
void Set_Time_and_Date()
{
  timeClient.setTimeOffset(timezone_offset);

  // Send an HTTP GET request
  timeClient.update();

  epochTime = timeClient.getEpochTime();
  currentHour = timeClient.getHours();
  currentMinute = timeClient.getMinutes();
  currentSecond = timeClient.getSeconds();
  weekDay = weekDays[timeClient.getDay()];
  // Get a time structure
  struct tm *ptm = gmtime((time_t *)&epochTime);
  monthDay = ptm->tm_mday;
  currentMonth = ptm->tm_mon + 1;
  currentMonthName = months[currentMonth - 1];
  currentYear = ptm->tm_year + 1900;
  currentYearforSet = currentYear - 2000;

  // Set time manually to 13:55:30 Jan 1st 2014
  // YOU CAN SET THE TIME FOR THIS EXAMPLE HERE
  tinfo.tm_year = currentYearforSet;
  tinfo.tm_mon = currentMonth;
  tinfo.tm_mday = monthDay;
  tinfo.tm_hour = currentHour;
  tinfo.tm_min = currentMinute;
  tinfo.tm_sec = currentSecond;

  // Convert time structure to timestamp
  initialt = timelib_make(&tinfo);

  // Set system time counter
  timelib_set(initialt);

  // Configure the library to update / sync every day (for hardware RTC)
  // timelib_set_provider(time_provider, TIMELIB_SECS_PER_DAY);
  Serial.println("Setting Date and Time from Internet is successful.");
}
