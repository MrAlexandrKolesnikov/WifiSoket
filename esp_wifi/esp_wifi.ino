//**************** include ********************//
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <SocketIoClient.h>
//**************** include ********************//

#define LED 2 //define LED pin 

const char* ssidAP = "I_ESP";                                    //Access pointer ssid
const char* passwordAP = "1234567890";                           //Access pointer password

char* ssid = new char[ 32 ];                                     //Wifi ssid
char* password = new char[ 64 ];                                 //Wifi password

bool isGetPassSSid = false;                                      //Receive password and ssid flag

SocketIoClient webSocket;                                        //websoket client object

IPAddress StaticIP(192 , 168 , 1 , 35 );                         //Static IP
IPAddress GateIP( 192 , 168 , 1 , 1 );                           //Gateway IP

WiFiServer server( 80 );                                         //Open port 



/**
* @brief  soketEvent
*         Function sockets event flow
* @param  payload:Received message
* @param  length:length of message
*/
void soketEvent(
           const char * payload ,                       
           size_t length                                
          ) 
{
  Serial.print( "Messege in:" );
  Serial.println( payload );                              
  
  String payloadString = payload;                                 //Convert message from char to string for more comfortable work
  
  if( payloadString.indexOf( "0" ) != -1 )                   
  {
    digitalWrite( LED , LOW );
  }
  else
  {
    digitalWrite( LED , HIGH );
  }
}

/**
* @brief  setup
*         setup and init all elements 
*/
void setup( ) 
{
  Serial.begin( 115200 );                                         //Init serial port
  delay(10);
  CreateWiFiAP( );                                                //Create wifi access pointer    
  delay(1500);    
  server.begin( );                                                //Start server on access pointer
                                                  
  ArduinoOTA.setHostname( "OTA->ESP" );                           //Start OTA server
  ArduinoOTA.begin( );
  
  pinMode( LED , OUTPUT );                                        //Init LED pin

  EEPROM.begin(100);
  Serial.print("Chip ID:");
  Serial.println(ESP.getChipId());
}


/**
* @brief  loop
*         main loop 
*/
void loop( ) 
{
   if( !isGetPassSSid )                                            //Check if we get password and ssid
   {
      if( ServerAP( ) )                                            //Wait until get pass and ssid
      {
        isGetPassSSid = true;                                      //Raise flag
        Serial.println( "Get password and SSID ok" );     
        if( !ConnectToWiFi( ) )                                    //Check connect to wifi
        {
          Serial.println( "Bad SSID or PASSWORD" );
          CreateWiFiAP( );                                          //If we can't connect to wifi againe create access pointer
          isGetPassSSid = false;                                    //Lower the flag
          EEPROM.write(97, 0);
          EEPROM.commit();
          return;
        }
          if(!EEPROM.read(97))
          {
            for(int i = 0 ; i < 32 ;i++) EEPROM.write(i, ssid[i]);
            for(int i = 0 ; i < 64 ;i++) EEPROM.write(32 + i, password[i]);
            EEPROM.write(97, 1);
            EEPROM.commit();
          }
          webSocket.begin( "tranquil-stream-82241.herokuapp.com" ); //Start soket
          webSocket.emit("indificate","\"id:esp1&identifier:wifi_power\"");
          webSocket.on( "news" , soketEvent );                      //Init soket event  
      }
   }
   else
   {
      webSocket.loop( );                                            //soket main loop
   }
}


/**
* @brief  ServerAP
*         Server function for respond to client and get ssid and password
* @retval Result of the operation: return "TRUE" if get ssid and password, else return "FALSE"
*/
bool ServerAP( )
{
  if(EEPROM.read(97) == 1)
  {
    Serial.println( "Use ssid and password from EEPROM");
    for(int i = 0 ; i < 32 ;i++) ssid[i] = EEPROM.read(i);
    for(int i = 0 ; i < 64 ;i++) password[i] = EEPROM.read(i + 32);
    Serial.println( ssid );
    Serial.println( password );
    return true;
  }
  ArduinoOTA.handle( );                                               //OTA server handle
  
  WiFiClient client = server.available( );                            //Checking the presence of the client
  if ( !client ) 
  {
    return false;
  }
  Serial.println( "Connect!" );

  
  while ( !client.available( ) )                                       //Wait until the client sends some data
  {
    ArduinoOTA.handle( );
    delay(1);
  }

  String req = client.readStringUntil( '\r' );                          // Read the first line of the request

  client.flush( );
  
  if ( req.indexOf( "=" ) != -1 )                                       //read password and ssid
  {
    int k = ( int )req.indexOf( "=" ) + 1;
    int i = 0;
    while( req[ k ] != '&' )
    {
      ssid[ i++ ] = req[ k++ ];
    }
    
    Serial.println( ssid );
    i = 0;
    k = ( int )req.lastIndexOf( "=" ) + 1;
    while(k != req.length( ) - 9 )
    {
       password[ i++ ] = req[ k++ ];
    }
    
    Serial.println( password );
    return true;
  }
  
  client.flush();
  
/**------------------------------------------HTML page for client---------------------------------**/
String html = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nCache-Control: private, no-store\r\n\r\n"
"<!DOCTYPE HTML>"
"<html>"
"  <head>"
"   <title>esp8266-12 config page </title>"
"   <style type=\"text/css\">"
"     h1 {font-size: 50px; font-family: impact}"
"   </style>"
" </head>"
" "
" <body>"
"   <hr />"
"   "
"   <h1 align=\"center\" style=\"color:white;\">esp8266-12 config page</h1>"
"   <body style=\"background-color:black;\">"
"   <form align=\"center\" name=\"forma1\">"
"          <p><input type=\"text\" name=\"log\" "
"                maxlength=\"32\" value=\"SSID\"></p>"
"          <p><input type=\"text\" name=\"pass\" "
"                maxlength=\"64\" value=\"PASSWORD\"></p> "
"     <p><button type=\"submit\">connect</button></p>"
"   </form>"
" </body>";
/**------------------------------------------END--------------------------------------------------**/

  client.print( html );                                                   // Send the response to the client
  delay(1);
  return false;
}

/**
* @brief  CreateWiFiAP
*         Function for init and create wifi access pointer
*/
void CreateWiFiAP( )
{
  WiFi.disconnect( );                                                     //Disconnect all client
  WiFi.mode( WIFI_AP );                                                   //Set wifi mode
  WiFi.softAPConfig( StaticIP , StaticIP , IPAddress(255, 255, 255, 0) ); //Config 
  WiFi.softAP( ssidAP , passwordAP );                                     //Create wifi access pointer
  Serial.println( "Access pointer was create" );
}


/**
* @brief  ConnectToWiFi
*         Function for connect to wifi
* @retval Result of the operation: return "TRUE" if connect successfully, else return "FALSE"
*/
bool ConnectToWiFi( )
{
  WiFi.disconnect( );                                                   //Disconnect all client
  WiFi.mode( WIFI_STA );                                                //Set wifi mode
  WiFi.softAPdisconnect( true );                                        //for fix stupid bug
  WiFi.enableAP( false );                                               //for fix stupid bug
  WiFi.softAPdisconnect( true );                                        //for fix stupid bug
  
  delay(500);
  WiFi.mode( WIFI_OFF );                                                //Set wifi mode
  delay(500);
  WiFi.mode( WIFI_STA );                                                //Set wifi mode
  delay(500);
  
  Serial.println( "Wi-Fi connecting" );
  
  WiFi.begin( ssid , password );                                        //try to connect to wifi
  
  int requestCounter = 0;
  
  while( WiFi.status() != WL_CONNECTED )                                //wait 10 seconds
  {
    Serial.print(".");
    delay(500);
    if( requestCounter == 20 ) return false;                             //check if time end
    requestCounter++;
  }
  Serial.print( "\nConnected!" );
  return true;
}
