#include "includes.h"

IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

EthernetClient ethClient;
PubSubClient client(ethClient);

void setup() {
  // Set up serial connection
  delay(2500);

  Serial.begin(SERIAL_BAUDRATE); 
  Serial.setTimeout(SERIAL_TIMEOUT);
  
  delay(1000);  

  // Set up LEDS
  LEDS.setBrightness(max_bright);

  // -- Single strip of 150 LEDS set for development testing ------------------------------- //
  
  LEDS.addLeds<LED_TYPE, LED_PIN_ONE, COLOR_ORDER>(leds, NUM_LEDS_PER_STRIP); 

  // --------------------------------------------------------------------------------------- // 

  set_max_power_in_volts_and_milliamps(5, 1000);
  
  // Set up variables
  random16_set_seed(4832);
  random16_add_entropy(analogRead(2));

  // Load starting mode and number of leds
  led_mode = EEPROM.read(STARTMODE);   

  // Set up palettes
  current_palette  = CRGBPalette16(CRGB::Black);
  target_palette   = RainbowColors_p;
  current_blending = LINEARBLEND;

  // Set up circ_noise variables
  for (long i = 0; i < NUM_LEDS_PER_STRIP; i++) {  
    uint8_t angle = (i * 256) / NUM_LEDS_PER_STRIP;  
    xd[i] = cos8( angle );                
    yd[i] = sin8( angle );               
  }

  client.setServer(mqtt_server, mqtt_port);
  Serial.println("MQTT SERVER SET");
  delay(2000);


  client.setCallback(callback);
  Serial.println("CALLBACK SET");
  delay(2000);
 
  // Init first mode
  strobe_mode(led_mode, 1);
}

void loop() {
  if (!client.connected()) {
    Serial.println("Reconnecting MQTT..."); 
    reconnectMqtt();
  }
 
  // Palette transitions - always running
  EVERY_N_MILLISECONDS(50) {
    uint8_t maxChanges = 24; 
    nblendPaletteTowardPalette(current_palette, target_palette, maxChanges);   
  }
  
  EVERY_N_SECONDS(5) {   
    if (palette_change == 1) SetupSimilar4Palette();
    if (palette_change == 2) SetupRandom4Palette();
    if (palette_change == 3) SetupRandom16Palette();
  }

  // Dynamically change delay
  EVERY_N_MILLIS_I(this_timer, this_delay) {
    this_timer.setPeriod(this_delay); 
    strobe_mode(led_mode, 0);
  }

  // Optionally add glitter
  if(glitter) addglitter(10); 

  FastLED.show(); 
}

// Callback used when an MQTT message is received
void callback(char* topic, byte* payload, unsigned int length) {
  char tmp[length+1];
  strncpy(tmp, (char*)payload, length);
  tmp[length] = '\0';
  String data(tmp);

  Serial.print("Received Data from Topic: ");
  Serial.println(data);

  if (data.length() > 0) {
    String command = getValue(data, ':', 0);
    Serial.print("Command: ");
    Serial.println(command);
    String value = getValue(data, ':', 1);
    Serial.print("Value: ");
    Serial.println(value);
	
    if (command.length() > 0) {
      if (command.equals("mode")) {
        strobe_mode(value.toInt(), 1);
      }
    }
  }
}

/*
 * Sets the mode/routine for the LEDs.
 *
 * @param newMode : the mode to set the leds to
 * @param mc      : signifies if we're changing modes or not
 */
void strobe_mode(uint8_t newMode, bool mc){

  // If this_ is a *new* mode, clear out LED array.
  if(mc) {
    fill_solid(leds, NUM_LEDS_PER_STRIP, CRGB( 0, 0, 0));
    Serial.print("Mode: "); 
    Serial.println(led_mode);
  }

  switch (newMode) {

    // 0 - all of
    case  0: 
      if(mc) { fill_solid(leds, NUM_LEDS_PER_STRIP, CRGB( 0, 0, 0 )); } 
      break;

    // 1 - all on
    case  1: 
      if(mc) { fill_solid(leds, NUM_LEDS_PER_STRIP, CRGB( 255, 255, 255 )); } 
      break;

    // 2 - two-sin
    case  2: 
      if(mc) { this_delay = 10; all_freq = 2; this_speed = 1; thatspeed = 1; this_hue = 0; thathue = 128; this_dir = 0; this_rot = 1; thatrot = 1; this_cutoff = 128; thatcutoff = 192; } 
      two_sin(); 
      break;

    // 3 - one-sin with rainbow pallete
    case  3: 
      if(mc) { this_delay = 20; target_palette = RainbowColors_p; all_freq = 4; bg_clr = 0; bg_bri = 0; this_bright = 255; start_index = 64; this_inc = 2; this_cutoff = 224; this_phase = 0; this_cutoff = 224; this_rot = 0; this_speed = 4; wave_brightness = 255; } 
      one_sin_pal(); 
      break;

    // 4 - noise8 with party palette
    case  4: 
      if(mc) { this_delay = 10; target_palette = PartyColors_p; palette_change = 2; } 
      noise8_pal(); 
      break;

    // 5 - two-sin
    case  5: 
      if(mc) { this_delay = 10; all_freq = 4; this_speed = -1; thatspeed = 0; this_hue = 64; thathue = 192; this_dir = 0; this_rot = 0; thatrot = 0; this_cutoff = 64; thatcutoff = 192; } 
      two_sin(); 
      break;

    // 6 - one-sin with rainbow palette
    case  6: 
      if(mc) { this_delay = 20; target_palette = RainbowColors_p; all_freq = 10; bg_clr = 64; bg_bri = 4; this_bright = 255; start_index = 64; this_inc = 2; this_cutoff = 224; this_phase = 0; this_cutoff = 224; this_rot = 0; this_speed = 4; wave_brightness = 255; } 
      one_sin_pal(); 
      break;

    // 7 - juggle mode
    case  7: 
      if(mc) { this_delay = 10; numdots = 2; target_palette = PartyColors_p; this_fade = 16; this_beat = 8; this_bright = 255; this_diff = 64; } 
      juggle_pal(); 
      break;

    // 8 - matrix with palette
    case  8: 
      if(mc) { this_delay = 40; target_palette = LavaColors_p; this_index = 128; this_dir = 1; this_rot = 0; this_bright = 255; bg_clr = 200; bg_bri = 6; } 
      matrix_pal(); 
      break;

    // 9 - two-sin
    case  9: 
      if(mc) { this_delay = 10; all_freq = 6; this_speed = 2; thatspeed = 3; this_hue = 96; thathue = 224; this_dir = 1; this_rot = 0; thatrot = 0; this_cutoff = 64; thatcutoff = 64; } 
      two_sin(); 
      break;

    // 10 - one-sin with rainbow palette
    case 10: 
      if(mc) { this_delay = 20; target_palette = RainbowColors_p; all_freq = 16; bg_clr = 0; bg_bri = 0; this_bright = 255; start_index = 64; this_inc = 2; this_cutoff = 224; this_phase = 0; this_cutoff = 224; this_rot = 0; this_speed = 4; wave_brightness = 255; } 
      one_sin_pal(); 
      break;

    // 11 - three-sin wiht palette
    case 11: 
      if(mc) { this_delay = 50; mul1 = 5; mul2 = 8; mul3 = 7; }
			three_sin_pal(); 
			break;
   
    // 12 - serendipitous with palette
    case 12:
      if(mc) { this_delay = 10; target_palette = ForestColors_p; }
			serendipitous_pal(); 
			break;
  
    // 13 - one-sine with lava palette
    case 13:
      if(mc) { this_delay = 20; target_palette = LavaColors_p; all_freq = 8; bg_clr = 0; bg_bri = 4; this_bright = 255; start_index = 64; this_inc = 2; this_cutoff = 224; this_phase = 0; this_cutoff = 224; this_rot = 0; this_speed = 4; wave_brightness = 255; }
			one_sin_pal(); 
			break;
 
    // 14 - two-sin
    case 14:
      if(mc) { this_delay = 10; all_freq = 20; this_speed = 2; thatspeed = -1; this_hue = 24; thathue = 180; this_dir = 1; this_rot = 0; thatrot = 1; this_cutoff = 64; thatcutoff = 128; }
			two_sin(); 
			break;

    // 15 - matrix with party palette
    case 15:
      if(mc) { this_delay = 50; target_palette = PartyColors_p; this_index = 64; this_dir = 0; this_rot = 1; this_bright = 255; bg_clr = 100; bg_bri = 10; }
			matrix_pal(); 
			break;

    // 16 - noise8 with palette
    case 16:
      if(mc) { this_delay = 10; target_palette = OceanColors_p; palette_change = 1; }
			noise8_pal(); 
			break;

    // 17 - circular noise with party palette
    case 17:
 			if(mc) { this_delay = 10; target_palette = PartyColors_p; }
			circnoise_pal_2(); 
			break;

    // 18 - two-sin
    case 18:
 			if(mc) { this_delay = 20; all_freq = 10; this_speed = 1; thatspeed = -2; this_hue = 48; thathue = 160; this_dir = 0; this_rot = 1; thatrot = -1; this_cutoff = 128; thatcutoff = 192; }
			two_sin(); 
			break;

    // 19 - three-sin with palette
    case 19:
 			if(mc) { this_delay = 50; mul1 = 6; mul2 = 9; mul3 = 11; }
			three_sin_pal(); 
			break;

    // 20 - rainbow march with wide waves
    case 20:
 			if(mc) { this_delay = 10; this_dir = 1; this_rot = 1; this_diff = 1; }
			rainbow_march(); 
			break;

    // 21 - rainbow march with narrow waves
    case 21:
 			if(mc) { this_delay = 10; this_dir = 1; this_rot = 2; this_diff = 10; }
			rainbow_march(); 
			break;

    // 22 - noise16 with palette
    case 22:
 			if(mc) { this_delay = 20; hxyinc = random16(1, 15); octaves = random16(1, 3); hue_octaves = random16(1, 5); hue_scale = random16(10,  50);  x = random16(); xscale = random16(); hxy =  random16(); hue_time = random16(); hue_speed = random16(1, 3); x_speed = random16(1, 30); }
			noise16_pal(); 
			break;

    // 23 - one-sine with ocean palette
    case 23:
 			if(mc) { this_delay = 20; target_palette = OceanColors_p; all_freq = 6; bg_clr = 0; bg_bri = 0; this_bright = 255; start_index = 64; this_inc = 2; this_cutoff = 224; this_phase = 0; this_cutoff = 224; this_rot = 0; this_speed = 4; wave_brightness = 255; }
			one_sin_pal(); 
			break;

    // 24 - circular noise with ocean palette
    case 24:
 			if(mc) { this_delay = 10; target_palette = OceanColors_p; }
			circnoise_pal_4(); 
			break;

    // 25 - confetti with party palette
    case 25:
 			if(mc) { this_delay = 20; target_palette = PartyColors_p; this_inc = 1; this_hue = 192; this_sat = 255; this_fade = 2; this_diff = 32; this_bright = 255; }
			confetti_pal(); 
			break;

    // 26 - two-sin
    case 26:
 			if(mc) { this_delay = 10; this_speed = 2; thatspeed = 3; this_hue = 96; thathue = 224; this_dir = 1; this_rot = 1; thatrot = 2; this_cutoff = 128; thatcutoff = 64; }
			two_sin(); 
			break;

    // 27 - matrix with forest palette
    case 27:
 			if(mc) { this_delay = 30; target_palette = ForestColors_p; this_index = 192; this_dir = 0; this_rot = 0; this_bright = 255; bg_clr = 50; bg_bri = 0; }
			matrix_pal(); 
			break;

    // 28 - one-sin with party palette
    case 28:
 			if(mc) { this_delay = 20; target_palette = RainbowColors_p; all_freq = 20; bg_clr = 0; bg_bri = 0; this_bright = 255; start_index = 64; this_inc = 2; this_cutoff = 224; this_phase = 0; this_cutoff = 224; this_rot = 0; this_speed = 4; wave_brightness = 255; }
			one_sin_pal(); 
			break;

    // 29 - confetti with lava palette
    case 29:
 			if(mc) { this_delay = 20; target_palette = LavaColors_p; this_inc = 2; this_hue = 128; this_fade = 8; this_diff = 64; this_bright = 255; }
			confetti_pal(); 
			break;

    // 30 - circular noise with party palette
    case 30:
 			if(mc) { this_delay = 10; target_palette = PartyColors_p; }
			circnoise_pal_3(); 
			break;

    // 31 - juggle mode with ocean palette
    case 31:
 			if(mc) { this_delay = 10; numdots = 4; target_palette = OceanColors_p; this_fade = 32; this_beat = 12; this_bright = 255; this_diff = 20; }
			juggle_pal(); 
			break;

    // 32 - one-sin with palette
    case 32:
 			if(mc) { this_delay = 30; SetupSimilar4Palette(); all_freq = 4; bg_clr = 64; bg_bri = 4; this_bright = 255; start_index = 64; this_inc = 2; this_cutoff = 224; this_phase = 0; this_cutoff = 128; this_rot = 1; this_speed = 8; wave_brightness = 255; }
			one_sin_pal(); 
			break;

    // 33 - three-sin with palette
    case 33:
 			if(mc) { this_delay = 50; mul1 = 3; mul2 = 4; mul3 = 5; }
			three_sin_pal(); 
			break;

    // 34 - rainbow march with slow, long waves
    case 34:
 			if(mc) { this_delay = 255; this_dir = -1; this_rot = 1; this_diff = 1;}
			rainbow_march(); 
			break;

    // 35 - circular noise with party palette
    case 35:
 			if(mc) { this_delay = 10; target_palette = PartyColors_p; }
			circnoise_pal_1(); 
			break;

    // 36 - confetti with forest palette
    case 36:
 			if(mc) { this_delay = 20; target_palette = ForestColors_p; this_inc = 1; this_hue = random8(255); this_fade = 1; this_bright = 255; }
			confetti_pal(); 
			break;

    // 37 - noise16 with palette
    case 37:
 			if(mc) { this_delay = 20; octaves = 1; hue_octaves = 2; hxy = 6000; x = 5000; xscale = 3000; hue_scale = 50; hue_speed = 15; x_speed = 100; }
			noise16_pal(); 
			break;

    // 38 - noise8 with lava palette
    case 38:
 			if(mc) { this_delay = 10; target_palette = LavaColors_p; palette_change = 0; }
			noise8_pal(); 
			break;

    // 39 - loading bar, then return to default mode
    case 39:
      if(mc) { this_delay = 10; target_palette = OceanColors_p; palette_change = 0; this_bright = 50; this_cutoff = 0; this_rot = 1; bg_clr = 64; }
      loading_bar_pal();
      break;
  }
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void reconnectMqtt() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect(mqtt_clientid, mqtt_user, mqtt_password)) {
      Serial.println("Connected");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("Failed, rc=");
      Serial.println(client.state());
      Serial.println(" Try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
