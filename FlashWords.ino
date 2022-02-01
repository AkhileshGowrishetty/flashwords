/*
  SD card connection
  The circuit:
   SD card attached to SPI bus as follows:
   // Arduino-pico core
   ** MISO - pin 16
   ** MOSI - pin 19
   ** CS   - pin 17
   ** SCK  - pin 18

   // Arduino-mbed core
   ** MISO - pin 4
   ** MOSI - pin 3
   ** CS   - pin 5
   ** SCK  - pin 2
*/


#if !defined(ARDUINO_ARCH_RP2040)
  #error For RP2040 only
#endif

#if defined(ARDUINO_ARCH_MBED)
  
  #define PIN_SD_MOSI       PIN_SPI_MOSI
  #define PIN_SD_MISO       PIN_SPI_MISO
  #define PIN_SD_SCK        PIN_SPI_SCK
  #define PIN_SD_SS         PIN_SPI_SS

#else

  #define PIN_SD_MOSI       PIN_SPI0_MOSI
  #define PIN_SD_MISO       PIN_SPI0_MISO
  #define PIN_SD_SCK        PIN_SPI0_SCK
  #define PIN_SD_SS         PIN_SPI0_SS
  
#endif

#define _RP2040_SD_LOGLEVEL_       0

#include <SPI.h>
#include <RP2040_SD.h>
#include <TFT_eSPI.h>
#include "Free_Fonts.h"



TFT_eSPI tft = TFT_eSPI();

File root;
int wordCount = 0;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  tft.init();
  tft.setCallback(pixelColor); 
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN ,TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Vocabulary", 160, 120, 4);
  tft.setTextSize(1);
  
  while (!Serial && millis() < 5000);

  delay(1000);

  Serial.println(BOARD_NAME);
  Serial.println(RP2040_SD_VERSION);
  
  Serial.print("Initializing SD card with SS = ");  Serial.println(PIN_SD_SS);
  Serial.print("SCK = ");   Serial.println(PIN_SD_SCK);
  Serial.print("MOSI = ");  Serial.println(PIN_SD_MOSI);
  Serial.print("MISO = ");  Serial.println(PIN_SD_MISO);

  if (!SD.begin(PIN_SD_SS)) {
    Serial.println("Initialization failed!");
    return;
  }

  Serial.println("Initialization done.");

  bool shouldCount = true;
  bool wordcountexists = false; 
  
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  if( SD.exists("count.txt") ) {
    wordcountexists = true;
  }
  
  if ( wordcountexists ) {
    Serial.println("wordcount file exists");
    File wordFile = SD.open("count.txt", FILE_READ );
    String n = wordFile.readStringUntil('\r');
    wordCount = n.toInt();
    wordFile.close();
    
    String newfilename = "words" + String(wordCount+1) + ".txt";
    Serial.println(newfilename);

    

    if( SD.exists(newfilename.c_str()) ) {
      shouldCount = true;
    }
    else {
      shouldCount = false;
    }
    
    
  }
  if ( shouldCount ) {
    wordCount = count();
    
    File wordFile = SD.open("count.txt", O_WRITE);
    char c[6];
    sprintf(c, "%d", wordCount);
    wordFile.write(c, strlen(c));
    Serial.println("wrote new word count");
    wordFile.close();
  }
  
  Serial.print("Number of words in this Card: "); Serial.println(wordCount);
  
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_YELLOW ,TFT_BLACK);
  tft.fillScreen(TFT_BLACK);
  String wc = "Words: " + String(wordCount);
  tft.drawString(wc, 160, 120, 4);
  delay(1000);
  randomSeed(millis());
}

void loop() {
  long randomNumber = random(1, wordCount-1);

  char filename[13];
  sprintf(filename, "%s%ld%s", "words", randomNumber, ".txt");

  File textFile = SD.open(filename, FILE_READ);

  if (textFile) {
    String text = textFile.readStringUntil('\r');
    
    int index1 = text.indexOf(": ");
    String word_ = text.substring(1, index1);
    
    int index2 = text.indexOf(", ");
    String partsSpeech = text.substring(index1+2, index2);
    partsSpeech.toUpperCase();    
    word_ = word_ + " (" + partsSpeech + ')';

    String meaning = "- " + text.substring(index2 + 2);
    
    Serial.println(text);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_YELLOW ,TFT_BLACK);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    tft.drawString(word_, 160, 10, 4);

    tft.setCursor(0, tft.fontHeight(4) + 30);
    tft.setTextFont(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.setFreeFont(FSS12);       
    printSplitString(meaning);

    int16_t y = tft.getCursorY();
    text = textFile.readStringUntil('\r');
    index1 = text.indexOf(": ");

    tft.setCursor(0, y + 30);
    tft.print("Synonyms: ");
    printSplitString(text.substring(index1+2));
    tft.unloadFont();
    
    textFile.close();
  }
  else {
    Serial.println("error opening file");
  }
  delay(10000);
}

unsigned int count()
{
  root = SD.open("/");
  unsigned count_ = 0;
  while(true) {
    File entry = root.openNextFile();

    if(!entry)
      break;

    if(strstr( entry.name(), "WORDS") && !entry.isDirectory() )
      count_++;

    entry.close();
  }

  return count_;
}

void printSplitString(String text)
{
  int wordStart = 0;
  int wordEnd = 0;
  while ( (text.indexOf(' ', wordStart) >= 0) && ( wordStart <= text.length())) {
    wordEnd = text.indexOf(' ', wordStart + 1);
    uint16_t len = tft.textWidth(text.substring(wordStart, wordEnd));
    if (tft.getCursorX() + len >= tft.width()) {
      tft.println();
      if (wordStart > 0) wordStart++;
    }
    tft.print(text.substring(wordStart, wordEnd));
    wordStart = wordEnd;
  }
}

uint16_t pixelColor(uint16_t x, uint16_t y) { return tft.readPixel(x, y);} 
