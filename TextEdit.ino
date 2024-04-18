#include <SD.h>
#include "USB.h"
#include <SPI.h>
#include "M5Cardputer.h"

#define display M5Cardputer.Display
#define kb M5Cardputer.Keyboard

File myFile;
String root = "/TextEdit";

int fileAmount;

int mainCursor = 0;
int scriptCursor = 0;

const int maxFiles = 100;

String sdFiles[maxFiles] = {"NEW FILE"};

const int ELEMENT_COUNT_MAX = 1000;
String fileText[ELEMENT_COUNT_MAX];

int cursorPosX, cursorPosY, screenPosX, screenPosY = 0;

int newFileLines = 0;

int letterHeight = 16;
int letterWidth = 12;

String cursor = "|";

String fileName;

bool creatingFile = false;
bool saveFile = false;
bool onFileFirstOpen = false;

void getDirectory() {
  fileAmount = 1;
  File dir = SD.open(root);

  while (true) {
    File entry =  dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }

    String fileName = entry.name();
    sdFiles[fileAmount] = fileName;
    fileAmount++;

    if (fileAmount > maxFiles) {
      display.println("Cannot store any more files");
    }

  }
  fileAmount--;
}

void printDirectory() {
  for (int i = 0; i <= fileAmount; i++) {
    display.drawString(sdFiles[i+screenPosY], 20, i*20);
  }
}

void handleMenus(int options, void (*executeFunction)(), int& cursor, void (*printMenu)()) {
  cursor = 0;
  while (true) {
    M5Cardputer.update();

    if (kb.isChange() || M5Cardputer.BtnA.wasReleased()) {
      display.fillScreen(BLACK);
      
      if (kb.isKeyPressed(';') && cursor > 0){
        cursor--;

        if (screenPosY > 0 && cursor > 0) {
          screenPosY--;
        }
      } else if (kb.isKeyPressed('.') && cursor < options) {
        cursor++;

        if (cursor * 20 >= display.height() - 20) {
          screenPosY++;
        }
      } else if (kb.isKeyPressed(',')) {
        mainMenu();
        break;
      }

      int drawCursor = cursor*20;

      if (cursor * 20 > display.height()-20) {
        drawCursor = (display.height() - 20) - 15;
      }

      display.setTextColor(WHITE);
        
      display.drawString(">", 5, drawCursor);

      printMenu();
      if (kb.isKeyPressed(KEY_ENTER)) {
        screenPosY = 0;
        delay(100);
        executeFunction();
        break;
      }
      
      if (M5Cardputer.BtnA.isPressed()) {
        display.fillScreen(BLACK);
        mainMenu();
        break;
      }
    }
  }
}

void openFile() {
  cleanNewFile();
  display.fillScreen(BLACK);
  display.setCursor(1,1);

  String fullFileName = sdFiles[mainCursor];
  String fullFilePath = root + "/" + fullFileName;
  myFile = SD.open(fullFilePath, FILE_READ);

  fileName = sdFiles[mainCursor].substring(0, fullFileName.length() - 4);

  while (myFile.available()) {
    String line = myFile.readStringUntil('\n');
    line.trim();
    fileText[newFileLines] = line;
    newFileLines++;
  }
  myFile.close();

  creatingFile = true;
  onFileFirstOpen = true; // Render the existing text when opened
}

void insertLine(String file[], int y){
  for (int i = newFileLines; i > y; i--){
    file[i] = file[i-1];
  }
  file[y] = '\0';
}

void insertChar(String file[], int y, int x, char letter){
  if (cursorPosX >= file[y].length()) {
    file[y] += letter;
  } else {
    String modLine = file[y].substring(0,x-1) + letter + file[y].substring(x-1,file[y].length());
    file[y] = modLine;
  }
}

void removeLine(String file[], int y){
  for (int i = y; i < newFileLines; i++) {
    file[i] = file[i+1];
  }
  file[newFileLines] = '\0';
  cursorPosY--;
  newFileLines--;
}

void cleanNewFile() {
  for (int i = 0; i <= newFileLines; i++) {
    fileText[i] = '\0';
  }
  cursorPosX = 0;
  cursorPosY = 0;
  screenPosX = 0;
  screenPosY = 0;
  newFileLines = 0;
}

void saveNewFile() {
  saveFile = false;
  myFile = SD.open(root + "/" + fileName + ".txt", FILE_WRITE);
  if (myFile) {
    for (int i = 0; i <= newFileLines; i++) {
      int textLen = fileText[i].length();
      char charText[textLen];
      strcpy(charText, fileText[i].c_str());

      unsigned char unChar[textLen];

      for (int x = 0; x < textLen; x++) {
        unChar[x] = static_cast<unsigned char>(charText[x]);
      }

      myFile.write(unChar, textLen);
      myFile.print('\n');

    }

    myFile.close();
    mainMenu();
  } else {
    display.println("File did not open");
    myFile.close();
  }
}

void newFile() {
  fileName = '\0';
  cleanNewFile();
  saveFile = false;
  display.fillScreen(BLACK);
  display.setCursor(1,1);
  creatingFile = true;
}

void deleteFile() {
  String fileName = root + "/" + sdFiles[mainCursor];
  display.fillScreen(BLACK);
  display.setCursor(1,1);
  display.println("Deleting file...");
  if (SD.remove(fileName)) {
    display.println("File deleted");
    sdFiles[fileAmount] = '\0';
    fileAmount--;
    delay(1000);
  } else {
    display.println("File could not be deleted");
    delay(1000);
  }
  mainMenu();

}

void scriptOptions() {
  if (scriptCursor == 0) {
    openFile();
  } else if (scriptCursor == 1) {
    deleteFile();
  }
}

void scriptMenu() {
  char optionsList[2][20] = {"Open file", "Delete file"};
    
  for (int i = 0; i < 2; i++) {
    display.setCursor(20,i*20);
    display.println(optionsList[i]);
  }
}

void mainOptions() {
  if (mainCursor == 0) {
    newFile(); 
  } else {
    handleMenus(1, &scriptOptions, scriptCursor, &scriptMenu);
  }
}

void mainMenu() {
  display.fillScreen(BLACK);
  display.setTextSize(2);
  display.setCursor(20,1);

  getDirectory();
  handleMenus(fileAmount, &mainOptions, mainCursor, &printDirectory);
}

void bootLogo(){
  display.fillScreen(BLACK);
  
  display.setTextSize(2);
  String ProgName = "TextEdit";

  display.setCursor(display.width()/2-(ProgName.length()/2)*letterWidth, display.height()/2 - 50);
  display.println(ProgName);

  display.setTextSize(1);
  display.setCursor(display.width()/2 - 90, display.height()/2 - 25);
  display.println("-. .-.   .-. .-.   .-. .-.   . ");
  display.setCursor(display.width()/2 - 90, display.height()/2 - 15);
  display.println("  \\   \\ /   \\   \\ /   \\   \\ / ");
  display.setCursor(display.width()/2 - 90, display.height()/2 - 5);
  display.println(" / \\   \\   / \\   \\   / \\   \\ ");
  display.setCursor(display.width()/2 - 90, display.height()/2 + 5);
  display.println("~   `-~ `-`   `-~ `-`   `-~ `- ");

  display.setTextSize(2);
  
  display.setCursor(display.width()/2 - 95, display.height()/2 + 40);
  display.println("Press any key...");

  while (true) {
    M5Cardputer.update();
    if (kb.isChange()) {
      delay(100);
      mainMenu();
      break;
    }
  }
}

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);

  SPI.begin(
    M5.getPin(m5::pin_name_t::sd_spi_sclk),
    M5.getPin(m5::pin_name_t::sd_spi_miso),
    M5.getPin(m5::pin_name_t::sd_spi_mosi),
    M5.getPin(m5::pin_name_t::sd_spi_ss));
  while (false == SD.begin(M5.getPin(m5::pin_name_t::sd_spi_ss), SPI)) {
    delay(1);
  }

  if (!SD.exists(root)) {
    SD.mkdir(root);
  }

  display.setRotation(1);
  display.setTextColor(WHITE);

  bootLogo();
}

void loop() {
  M5Cardputer.update();

  if (saveFile) {
    if (kb.isChange()) {
      if (kb.isPressed()) {
        Keyboard_Class::KeysState status = kb.keysState();

        for (auto i : status.word) {
          fileName += i;
        }

        if (status.del) {
          fileName.remove(fileName.length() - 1);  
        }

        display.fillScreen(BLACK);
        display.setCursor(display.width()/2-5*letterWidth, 0);
        display.println("File Name:");
        display.drawString(fileName, display.width()/2-(fileName.length()/2)*letterWidth, letterHeight);

        if (status.enter) {
          saveNewFile();
        }
      }
    }
  }

  if (creatingFile) {
    if (M5Cardputer.BtnA.isPressed()) {
      creatingFile = false;
      display.fillScreen(BLACK);
      mainMenu();
    }

    if (kb.isChange() || onFileFirstOpen) {
      if (kb.isPressed() || onFileFirstOpen) {
        Keyboard_Class::KeysState status = kb.keysState();
        int prevCursorY;

        // Save the file
        if (status.fn && kb.isKeyPressed('`')) {
          creatingFile = false;
          saveFile = true;
          
        // Move cursor up
        } else if (status.fn && kb.isKeyPressed(';') && cursorPosY > 0){
          prevCursorY = cursorPosY;
          cursorPosY--;
          if (cursorPosX >= fileText[prevCursorY].length()) {
            cursorPosX = fileText[cursorPosY].length();
          }
          if (screenPosY > 0 && cursorPosY > 0) {
            screenPosY--;
          }
          if (cursorPosX * letterWidth > display.width()) {
            screenPosX = (fileText[cursorPosY].length() - 19) * -letterWidth;
        } else {
          screenPosX = 0;
        }

        // Move cursor down
        } else if (status.fn && kb.isKeyPressed('.') && cursorPosY < newFileLines) {
          prevCursorY = cursorPosY;
          cursorPosY++;
          if (cursorPosX >= fileText[prevCursorY].length()) {
            cursorPosX = fileText[cursorPosY].length();
          }
          if (cursorPosY * letterHeight >= display.height() - letterHeight) {
            screenPosY++;
          }
          if (cursorPosX * letterWidth > display.width()) {
            screenPosX = (fileText[cursorPosY].length() - 19) * -letterWidth;
          } else {
            screenPosX = 0;
          }

        // Move cursor left
        } else if (status.fn && kb.isKeyPressed(',') && cursorPosX > 0) {
          cursorPosX--;
          if (screenPosX < 0) {
            screenPosX += letterWidth;
          }

        // Move cursor right
        } else if (status.fn && kb.isKeyPressed('/') && cursorPosX < fileText[cursorPosY].length() ) {
          cursorPosX++;
          if (cursorPosX * letterWidth >= display.width()) {
            screenPosX -= letterWidth;
          }

        // Else insert character
        } else if (!status.fn) {
          for (auto i : status.word) {

            if (cursorPosX * letterWidth >= display.width() - letterWidth) {
              screenPosX -= letterWidth;
            }
            cursorPosX++;

            insertChar(fileText, cursorPosY, cursorPosX, i);
          }
          
        }

        // Delete character
        if (status.del) {

          if (screenPosX < 0) {
            screenPosX += letterWidth;
          }
          if (cursorPosX > 0) {
            cursorPosX--;
          }

          if (fileText[cursorPosY].length() <= 0 && cursorPosY > 0) {
            removeLine(fileText, cursorPosY);
            if (fileText[cursorPosY].length() * letterWidth > display.width()) {
              screenPosX = (fileText[cursorPosY].length() - 19) * -letterWidth;
            } else {
              screenPosX = 0;
            }
          } else {
            fileText[cursorPosY].remove(cursorPosX,1);
          }
        }

        // New line
        if (status.enter) {
          newFileLines++;
          cursorPosY++;
          
          insertLine(fileText, cursorPosY);

          if (cursorPosY * letterHeight >= display.height()-letterHeight) {
            screenPosY++;
          }
          screenPosX = 0;
          cursorPosX = 0;
        }

        // Set cursor back to EOF if it moves beyond it
        if (cursorPosX > fileText[cursorPosY].length()) {
          cursorPosX = fileText[cursorPosY].length();
        }

        display.fillScreen(BLACK);

        display.setTextSize(1.5);

        int drawCursorX = cursorPosX * letterWidth;
        int drawCursorY = cursorPosY * letterHeight - 5;

        if (cursorPosX * letterWidth > display.width() - letterWidth) {
          drawCursorX = display.width() - letterWidth;
        }

        if (cursorPosY * letterHeight > display.height() - letterHeight) {
          drawCursorY = (display.height() - letterHeight) - 10;
        }

        display.drawString(cursor, drawCursorX - 3, drawCursorY, &fonts::Font2);
        
        display.setTextSize(2);

        for (int i = 0; i<= newFileLines; i++) {
          display.drawString(fileText[i+screenPosY], screenPosX, i*letterHeight);
        }

        onFileFirstOpen = false;
      }
    }
  }
} 