#include "Common.h"
#include "Utils.h"
#include "Menu.h"
#include "Themes.h"
#include "Draw.h"

// newkirks recommended 200ms delay
#define SBATS_POLL_TIME    200 // Tuning status polling interval (msecs)

static uint32_t sbatsTime = millis();
static uint32_t sbatsPress = 0;
static uint16_t sbatsCurFreq;
static uint16_t sbatsMinFreq;
static uint16_t sbatsMaxFreq;
static uint16_t sbatsStep;
static bool     sbatsRev = false;

static void sbatsInit(uint16_t freq) {
  const Band *band = getCurrentBand();
  const Step *step = getCurrentStep();
  sbatsCurFreq      = freq;
  sbatsMinFreq      = band->minimumFreq;
  sbatsMaxFreq      = band->maximumFreq;
  sbatsStep         = step->step;
  sbatsTime         = millis();
  sbatsPress        = 0;
  if(band->bandName == "VHF") {           // override FM band limits for north america
    sbatsMinFreq    = 8710;
    sbatsMaxFreq    = 10790;
  } else if(band->bandName == "MW1") {    // override AM band limits for north america
    sbatsMinFreq    = 500;
  }
  // clear parts of the screen  (screen size 320 x 170)
  spr.fillRect(90, 0, 240, 15, TH.bg);     // above band/mode, right of the sidebar
  spr.fillRect(90, 90, 240, 80, TH.bg);    // under frequency, right of sidebar
  spr.fillRect(0, 115, 320, 55, TH.bg);    // bottom area, full width
  spr.pushSprite(0, 0);
  delay(10);
}

static bool sbatsMainLoop() {
  // check for button activity
  if(sbatsPress) {                                   // encoder previously pressed
    if(digitalRead(ENCODER_PUSH_BUTTON)==HIGH) {     // encoder released
      uint32_t delta = millis() - sbatsPress;
      if(delta > 500) {                              // longish press
        // cancel SB-ATS
        currentCmd = CMD_NONE;                       // cancel SBATS mode
      } else if(delta > 20) {                        // click
        // change direction
        sbatsRev = !sbatsRev;                        // reverse direction
      }
      sbatsPress=0;                                  // clear press
      delay(5);
    }
  } else if(digitalRead(ENCODER_PUSH_BUTTON)==LOW) { // encoder pressed
    sbatsPress = millis();                           // set encoder pressed
    delay(5);
  }
  // SB-ATS must be the active command
  if(currentCmd != CMD_SBATS) return(false);
  // Wait for the right time
  if(millis() - sbatsTime < SBATS_POLL_TIME) return(true);
  // check the tuning status, make sure its finished what it was doing
  rx.getStatus(0, 0);
  if(!rx.getTuneCompleteTriggered()) {
    sbatsTime = millis(); // reset the clock
    return(true);
  }
  // if we're already tuned to the desired freq then increment
  if(rx.getCurrentFrequency() == sbatsCurFreq) {
    if(sbatsRev) {
      sbatsCurFreq -= sbatsStep;
      if(sbatsCurFreq < sbatsMinFreq) sbatsCurFreq = sbatsMaxFreq;
    } else {
      sbatsCurFreq += sbatsStep;
      if(sbatsCurFreq > sbatsMaxFreq) sbatsCurFreq = sbatsMinFreq;
    }
  }
  // set the tuner to the new / correct frequency
  rx.setFrequency(sbatsCurFreq);
  // now update the display to show we've done something
  spr.fillRect(100, 35, 152, 52, TH.bg);
  spr.fillRect(0, 115, 320, 55, TH.bg);
  drawFrequency(sbatsCurFreq, FREQ_OFFSET_X, FREQ_OFFSET_Y, FUNIT_OFFSET_X, FUNIT_OFFSET_Y, 100);
  drawScale(sbatsCurFreq);
  spr.pushSprite(0, 0);
  sbatsTime = millis(); // reset the clock
  return(true);
}

// Run sb-ats
void sbatsRun(uint16_t originalFreq) {
  // initialize SB-ATS
  sbatsInit(originalFreq);
  // run the main loop until it returns false
  while(sbatsMainLoop());
  // Restore current frequency
  rx.setFrequency(originalFreq);
  currentCmd=CMD_NONE;
  drawScreen();
}
