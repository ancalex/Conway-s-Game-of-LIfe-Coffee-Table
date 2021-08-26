/*
 * * ESP8266 template with phone config web page
 * based on BVB_WebConfig_OTA_V7 from Andreas Spiess https://github.com/SensorsIot/Internet-of-Things-with-ESP8266
 *
 */
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#include "FastLED.h"

#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define NUM_STRIPS 5
#define NUM_LEDS_PER_STRIP 125
#define NUM_LEDS NUM_LEDS_PER_STRIP * NUM_STRIPS
CRGB leds[NUM_STRIPS * NUM_LEDS_PER_STRIP];
const uint8_t MatrixWidth = 25;
const uint8_t MatrixHeight = 25;
uint8 BRIGHTNESS = 64;
//CRGBPalette16 currentPalette = fire_gp;
int generation = 0;

class Cell {
public:
	bool alive =  1;
	bool prev =  1;
	uint8_t color_index = 0;
};
Cell world[MatrixWidth][MatrixHeight];

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include <EEPROM.h>
#include "global.h"
#include "gradients.h"
#include "NTP.h"

// Include STYLE and Script "Pages"
#include "Page_Script.js.h"
#include "Page_Style.css.h"

// Include HTML "Pages"
#include "Page_Admin.h"
#include "Page_NTPSettings.h"
#include "Page_Information.h"
#include "Page_NetworkConfiguration.h"
#include "Page_SetTime.h"
#include "Page_Games.h"

CRGBPalette16 currentPalette = fire_gp;

extern "C" {
#include "user_interface.h"
}

void setup() {

	Serial.begin(115200);

	//**** Network Config load
	EEPROM.begin(512); // define an EEPROM space of 512Bytes to store data
	CFG_saved = ReadConfig();

	//  Connect to WiFi acess point or start as Acess point
	if (CFG_saved)  //if no configuration yet saved, load defaults
	{
		// Connect the ESP8266 to local WIFI network in Station mode
		Serial.println("Booting");
		//printConfig();
		WiFi.mode(WIFI_STA);
		WiFi.begin(config.ssid.c_str(), config.password.c_str());
		WIFI_connected = WiFi.waitForConnectResult();
		if (WIFI_connected != WL_CONNECTED)
			Serial.println("Connection Failed! activating the AP mode...");

		Serial.print("Wifi ip:");
		Serial.println(WiFi.localIP());
	}

	if ((WIFI_connected != WL_CONNECTED) or !CFG_saved) {
		// DEFAULT CONFIG
		Serial.println("Setting AP mode default parameters");
		config.ssid = "GoL_Coffee_Table-" + String(ESP.getChipId(), HEX); // SSID of access point
		config.password = "";   // password of access point
		config.dhcp = true;
		config.IP[0] = 192;
		config.IP[1] = 168;
		config.IP[2] = 1;
		config.IP[3] = 100;
		config.Netmask[0] = 255;
		config.Netmask[1] = 255;
		config.Netmask[2] = 255;
		config.Netmask[3] = 0;
		config.Gateway[0] = 192;
		config.Gateway[1] = 168;
		config.Gateway[2] = 1;
		config.Gateway[3] = 254;
		config.DeviceName = "GoL_Coffee_Table";
		config.ntpServerName = "0.ro.pool.ntp.org"; // to be adjusted to PT ntp.ist.utl.pt
		config.Update_Time_Via_NTP_Every = 3;
		config.timeZone = 20;
		config.isDayLightSaving = true;
		config.density = 50;
		config.fading_step = 30;
		config.game = 0;
		//WriteConfig();
		WiFi.mode(WIFI_AP);
		WiFi.softAP(config.ssid.c_str(),"admin1234");
		Serial.print("Wifi ip:");
		Serial.println(WiFi.softAPIP());
	}

	// Start HTTP Server for configuration
	server.on("/", []() {
		Serial.println("admin.html");
		server.send_P ( 200, "text/html", PAGE_AdminMainPage); // const char top of page
	});

	server.on("/favicon.ico", []() {
		Serial.println("favicon.ico");
		server.send( 200, "text/html", "" );
	});
	//server.on("/schedule.html", send_schedule_settings_html);
	server.on("/games.html", send_games_html);
	// Network config
	server.on("/config.html", send_network_configuration_html);
	// Info Page
	server.on("/info.html", []() {
		Serial.println("info.html");
		server.send_P ( 200, "text/html", PAGE_Information );
	});
	server.on("/ntp.html", send_NTP_configuration_html);
	server.on("/time.html", send_Time_Set_html);
	server.on("/style.css", []() {
		Serial.println("style.css");
		server.send_P ( 200, "text/plain", PAGE_Style_css );
	});
	server.on("/microajax.js", []() {
		Serial.println("microajax.js");
		server.send_P ( 200, "text/plain", PAGE_microajax_js );
	});
	server.on("/admin/values", send_network_configuration_values_html);
	server.on("/admin/connectionstate", send_connection_state_values_html);
	server.on("/admin/infovalues", send_information_values_html);
	server.on("/admin/ntpvalues", send_NTP_configuration_values_html);
	server.on("/admin/timevalues", send_Time_Set_values_html);
	server.on("/admin/gamesvalues", send_games_values_html);
	server.onNotFound([]() {
		Serial.println("Page Not Found");
		server.send ( 400, "text/html", "Page not Found" );
	});
	server.begin();
	Serial.println("HTTP server started");

	printConfig();

	// start internal time update ISR
	tkSecond.attach(1, ISRsecondTick);

	// tell FastLED about the LED strip configuration
	FastLED.addLeds<LED_TYPE, 4>(leds, 0, NUM_LEDS_PER_STRIP).setDither(BRIGHTNESS < 255); // cpt-city palettes have different color balance
	FastLED.addLeds<LED_TYPE, 5>(leds, NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP).setDither(BRIGHTNESS < 255);
	FastLED.addLeds<LED_TYPE, 12>(leds, 2*NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP).setDither(BRIGHTNESS < 255);
	FastLED.addLeds<LED_TYPE, 13>(leds, 3*NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP).setDither(BRIGHTNESS < 255);
	FastLED.addLeds<LED_TYPE, 14>(leds, 4*NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP).setDither(BRIGHTNESS < 255);
	FastLED.setBrightness(BRIGHTNESS);

	Serial.println("FastLed Setup done");

	// start internal time update ISR
	tkSecond.attach(1, ISRsecondTick);
}

// the loop function runs over and over again forever
void loop() {
	server.handleClient();

	if (config.Update_Time_Via_NTP_Every > 0) {
		if (cNTP_Update > 5 && firstStart) {
			getNTPtime();
            delay(1500); //wait for DateTime
			cNTP_Update = 0;
			firstStart = false;
		}
		else if (cNTP_Update > (config.Update_Time_Via_NTP_Every * 60)) {
			getNTPtime();
			cNTP_Update = 0;
		}
	}
	//  feed de DOG :)
	customWatchdog = millis();

	//============================
	if (WIFI_connected != WL_CONNECTED and manual_time_set == false) {
		config.Update_Time_Via_NTP_Every = 0;
		//display_animation_no_wifi();
		softtwinkles();
		//display();
	} else if (ntp_response_ok == false and manual_time_set == false) {
		config.Update_Time_Via_NTP_Every = 1;
		//display_animation_no_ntp();
		pride();
		//display();
	} else if (ntp_response_ok == true or manual_time_set == true) {
		display();
	}
	FastLED.show();
}

void display() {
	server.handleClient();
	if (generation == 0) {
		fill_solid((CRGB*)leds, NUM_LEDS, CRGB::Black);
		randomFillWorld();
		chooseNewPalette();
	}

	// Display current generation
	for (int i = 0; i < MatrixWidth; i++) {
		for (int j = 0; j < MatrixHeight; j++) {
			if (world[i][j].alive == 1) {
				leds[XY(i, j)] = ColorFromPalette(currentPalette, world[i][j].color_index);
			}
		}
	}
	FastLED.show();
	for (int k = 0; k < BRIGHTNESS; k = k + config.fading_step) {
		for (int i = 0; i < MatrixWidth; i++) {
			for (int j = 0; j < MatrixHeight; j++) {
				if ( world[i][j].alive == 0){
					leds[XY(i, j)].fadeToBlackBy(k);
				}
			}
		}
		FastLED.show();
	}

	switch (config.game) {
	case 0:
		conway();
		break;
	case 1:
		amoeba();
		break;
	case 2:
		assimilation();
		break;
	case 3:
		coral();
		break;
	case 4:
		highlife();
		break;
	case 5:
		gnarl();
		break;
	case 6:
		longlife();
		break;
	case 7:
		replicator();
		break;
	}

	// Copy next generation into place
	for (int x = 0; x < MatrixWidth; x++) {
		for (int y = 0; y < MatrixHeight; y++) {
			world[x][y].prev = world[x][y].alive;
		}
	}

	generation++;
	if (generation >= 255) {
		generation = 0;
	}
}

void randomFillWorld() {
	for (int i = 0; i < MatrixWidth; i++) {
		for (int j = 0; j < MatrixHeight; j++) {
			if (random(100) < (unsigned long)config.density) {
				world[i][j].alive = 1;
				world[i][j].color_index = 0;
			}
			else {
				world[i][j].alive = 0;
			}
			world[i][j].prev = world[i][j].alive;
		}
	}
}
//
int neighbours(int x, int y) {
	return (world[(x + 1) % MatrixWidth][y].prev) +
			(world[x][(y + 1) % MatrixHeight].prev) +
			(world[(x + MatrixWidth - 1) % MatrixWidth][y].prev) +
			(world[x][(y + MatrixHeight - 1) % MatrixHeight].prev) +
			(world[(x + 1) % MatrixWidth][(y + 1) % MatrixHeight].prev) +
			(world[(x + MatrixWidth - 1) % MatrixWidth][(y + 1) % MatrixHeight].prev) +
			(world[(x + MatrixWidth - 1) % MatrixWidth][(y + MatrixHeight - 1) % MatrixHeight].prev) +
			(world[(x + 1) % MatrixWidth][(y + MatrixHeight - 1) % MatrixHeight].prev);
}

uint16_t XY( uint8_t x, uint8_t y)
{
	uint16_t i;
	if (y==1 || y==3 || y==6 || y==8 || y==11 || y==13 || y==16 || y==18 || y==21 || y==23) {
		i = y*25+(24-x);
	}
	else {
		i = y*25+x;
	}
	return i;
}

void chooseNewPalette() {
	switch(random(0, 8)) {
	case 0:
		currentPalette = abstract_3_gp;
		break;

	case 1:
		currentPalette = rainbowsherbet_gp;
		break;

	case 2:
		currentPalette = grayscale09_gp;
		break;

	case 3:
		currentPalette = Gummy_Kids_gp;
		break;

	case 4:
		currentPalette = ib11_gp;
		break;

	case 5:
		currentPalette = xmas_19_gp;
		break;

	case 6:
		currentPalette = pastel_rainbow_gp;
		break;

	case 7:
	default:
		currentPalette = fire_gp;
		break;
	}
}
//code from https://gist.github.com/kriegsman/964de772d64c502760e5
void pride()
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;

  for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV( hue8, sat8, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS-1) - pixelnumber;

    nblend( leds[pixelnumber], newcolor, 64);
  }
}

//code from https://gist.github.com/kriegsman/99082f66a726bdff7776
const CRGB lightcolor(8,5,1);

void softtwinkles() {
  for( int i = 0; i < NUM_LEDS; i++) {
    if( !leds[i]) continue; // skip black pixels
    if( leds[i].r & 1) { // is red odd?
      leds[i] -= lightcolor; // darken if red is odd
    } else {
      leds[i] += lightcolor; // brighten if red is even
    }
  }
  // Randomly choose a pixel, and if it's black, 'bump' it up a little.
  // Since it will now have an EVEN red component, it will start getting
  // brighter over time.
  if( random8() < 60) {
    int j = random16(NUM_LEDS);
    if( !leds[j] ) leds[j] = lightcolor;
  }
}
//Games
void conway() {
	// Birth and death cycle - Conway's Game of Life, density 50, fading_step 30
	for (int x = 0; x < MatrixWidth; x++) {
		for (int y = 0; y < MatrixHeight; y++) {
			// Default is for cell to stay the same
			if (world[x][y].prev == 0){
				world[x][y].color_index += 1;
			}
			int ncount = neighbours(x, y);
			if ((ncount == 3) && world[x][y].prev == 0 ) {
				// A new cell is born
				world[x][y].alive = 1;
				world[x][y].color_index += 2;
			}
			else if (((ncount < 2) || (ncount > 3)) && world[x][y].prev == 1) {
				// Cell dies
				world[x][y].alive = 0;
			}
		}
	}
}
void amoeba() {
	// Birth and death cycle - Amoeba, density 50, fading_step 6
	for (int x = 0; x < MatrixWidth; x++) {
		for (int y = 0; y < MatrixHeight; y++) {
			// Default is for cell to stay the same
			if (world[x][y].prev == 0){
				world[x][y].color_index += 1;
			}
			int ncount = neighbours(x, y);
			if ((ncount == 3 || (ncount == 5) || (ncount == 7)) && world[x][y].prev == 0 ) {
				// A new cell is born
				world[x][y].alive = 1;
				world[x][y].color_index += 2;
			}
			else if (((ncount == 2) || (ncount == 4) || (ncount == 6) || (ncount == 7)) && world[x][y].prev == 1) {
				// Cell dies
				world[x][y].alive = 0;
			}
		}
	}
}
void assimilation() {
	// Birth and death cycle - Assimilation, density 25, fading_step 6
	for (int x = 0; x < MatrixWidth; x++) {
		for (int y = 0; y < MatrixHeight; y++) {
			// Default is for cell to stay the same
			if (world[x][y].prev == 0){
				world[x][y].color_index += 1;
			}
			int ncount = neighbours(x, y);
			if ((ncount == 3 || (ncount == 4) || (ncount == 5)) && world[x][y].prev == 0 ) {
				// A new cell is born
				world[x][y].alive = 1;
				world[x][y].color_index += 2;
			}
			else if (((ncount == 1) || (ncount == 2) || (ncount == 3) || (ncount == 8)) && world[x][y].prev == 1) {
				// Cell dies
				world[x][y].alive = 0;
			}
		}
	}
}
void coral() {
	// Birth and death cycle - Coral, density 50, fading_step 6
	for (int x = 0; x < MatrixWidth; x++) {
		for (int y = 0; y < MatrixHeight; y++) {
			// Default is for cell to stay the same
			if (world[x][y].prev == 0){
				world[x][y].color_index += 1;
			}
			int ncount = neighbours(x, y);
			if ((ncount == 3 ) && world[x][y].prev == 0 ) {
				// A new cell is born
				world[x][y].alive = 1;
				world[x][y].color_index += 2;
			}
			else if (((ncount == 1) || (ncount == 2) || (ncount == 3)) && world[x][y].prev == 1) {
				// Cell dies
				world[x][y].alive = 0;
			}
		}
	}
}
void highlife() {
	// Birth and death cycle - HighLife, density 50, fading_step 6
	for (int x = 0; x < MatrixWidth; x++) {
		for (int y = 0; y < MatrixHeight; y++) {
			// Default is for cell to stay the same
			if (world[x][y].prev == 0){
				world[x][y].color_index += 1;
			}
			int ncount = neighbours(x, y);
			if ((ncount == 3 || ncount == 6) && world[x][y].prev == 0 ) {
				// A new cell is born
				world[x][y].alive = 1;
				world[x][y].color_index += 2;
			}
			else if ((ncount < 2 || ncount > 3) && world[x][y].prev == 1) {
				// Cell dies
				world[x][y].alive = 0;
			}
		}
	}

}
void gnarl() {
	// Birth and death cycle - Gnarl, density 8, fading_step 12
	for (int x = 0; x < MatrixWidth; x++) {
		for (int y = 0; y < MatrixHeight; y++) {
			// Default is for cell to stay the same
			if (world[x][y].prev == 0){
				world[x][y].color_index += 1;
			}
			int ncount = neighbours(x, y);
			if ((ncount == 1) && world[x][y].prev == 0 ) {
				// A new cell is born
				world[x][y].alive = 1;
				world[x][y].color_index += 2;
			}
			else if ((ncount < 1 || ncount > 1) && world[x][y].prev == 1) {
				// Cell dies
				world[x][y].alive = 0;
			}
		}
	}
}
void longlife() {
	// Birth and death cycle - LongLife, density 50, fading_step 6
	for (int x = 0; x < MatrixWidth; x++) {
		for (int y = 0; y < MatrixHeight; y++) {
			// Default is for cell to stay the same
			if (world[x][y].prev == 0){
				world[x][y].color_index += 1;
			}
			int ncount = neighbours(x, y);
			if ((ncount == 3 || ncount == 4 || ncount == 5) && world[x][y].prev == 0 ) {
				// A new cell is born
				world[x][y].alive = 1;
				world[x][y].color_index += 2;
			}
			else if ((ncount < 5 || ncount > 5) && world[x][y].prev == 1) {
				// Cell dies
				world[x][y].alive = 0;
			}
		}
	}
}
void replicator() {
	// Birth and death cycle - Replicator, density 2, fading_step 8
	for (int x = 0; x < MatrixWidth; x++) {
		for (int y = 0; y < MatrixHeight; y++) {
			// Default is for cell to stay the same
			if (world[x][y].prev == 0){
				world[x][y].color_index += 1;
			}
			int ncount = neighbours(x, y);
			if ((ncount == 1 || ncount == 3 || ncount == 5 || ncount == 7) && world[x][y].prev == 0 ) {
				// A new cell is born
				world[x][y].alive = 1;
				world[x][y].color_index += 2;
			}
			else if ((ncount = 2 || ncount == 4 || ncount == 6 || ncount == 8) && world[x][y].prev == 1) {
				// Cell dies
				world[x][y].alive = 0;
			}
		}
	}
}
