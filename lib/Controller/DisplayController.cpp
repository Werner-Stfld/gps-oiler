#include <Complex.h>

#include "globals.h"
#include "webController.h"
#include "DisplayController.h"
#include "icons.h"


void DisplayController::setup() {
    restore();

    tft.init(INITR_GREENTAB2);
    delay(150);
    tft.setRotation(1);
   tft.fillScreen(TFT_WHITE);
   spr.createSprite(TFT_HEIGHT, TFT_WIDTH);  // Fullscreen
   currentScreen()->setup();
}

DisplayController::DisplayController() {
    add(&currScreen);
    add(&brightness);
    add(&timeZone);

    brightnessScreen.execute = setBrightness;
};

void DisplayController::loop(bool pressed)
{
    buttonHandler.loop(pressed);
    ButtonState buttonState = buttonHandler.GetButtonState();
    NextScreenAction screenAction = currentScreen()->ScreenAction(buttonState);
    if (screenAction == NextScreenAction::nextScreen) {
        int no = currScreen.get();
        no++;
        if (no >= numScreens) no = 0;
        currScreen.set(no, no < 2?SetMode::flush:SetMode::cache); // Persist only default and inverted default screen.
        currentScreen()->setup();
    }
    if (screenAction == NextScreenAction::defaultScreen) {
        currScreen.set(0, SetMode::flush);
        currentScreen()->setup();
    }
    ScreenArgs args;
    args.timeToActionInPercent = buttonHandler.TimeToActionInPercent();
    currentScreen()->loop(args);
}
