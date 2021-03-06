/*
 *************************************************************
 *                      NEXTSTEPRC                           *
 *                                                           *
 *             -> Build your DIY MEGA 2560 TX                *
 *                                                           *
 *      Based on code named                                  *
 *      OpenTx - https://github.com/opentx/opentx            *
 *                                                           *
 *         Only avr code here for lisibility ;-)             *
 *                                                           *
 *  License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html  *
 *                                                           *
 *************************************************************
 */

#include "../../opentx.h"

#define STATUS_BAR_Y     (7*FH+1)
#define TELEM_2ND_COLUMN (10*FW)

#if defined(FRSKY_HUB) && defined(GAUGES)
bar_threshold_t barsThresholds[THLD_MAX];
#endif

uint8_t s_frsky_view = 0;

#define BAR_LEFT    25
#define BAR_WIDTH   100

void displayRssiLine()
{
  if (TELEMETRY_STREAMING()) {
    lcdDrawSolidHorizontalLine(0, 55, 128, 0); // separator
    uint8_t rssi;
    rssi = min((uint8_t)99, frskyData.rssi[1].value);
    lcdDrawTextLeft(STATUS_BAR_Y, STR_TX); lcdDrawNumberNAtt(4*FW+1, STATUS_BAR_Y, rssi, LEADING0, 2);
    lcdDrawRect(BAR_LEFT+1, 57, 38, 7);
    lcdDrawFilledRect(BAR_LEFT+1, 58, 4*rssi/11, 5, (rssi < getRssiAlarmValue(0)) ? DOTTED : SOLID);
    rssi = min((uint8_t)99, TELEMETRY_RSSI());
    lcdDrawText(104, STATUS_BAR_Y, STR_RX); lcdDrawNumberNAtt(105+4*FW, STATUS_BAR_Y, rssi, LEADING0, 2);
    lcdDrawRect(65, 57, 38, 7);
    uint8_t v = 4*rssi/11;
    lcdDrawFilledRect(66+36-v, 58, v, 5, (rssi < getRssiAlarmValue(0)) ? DOTTED : SOLID);
  }
  else {
    lcdDrawTextAtt(7*FW, STATUS_BAR_Y, STR_NODATA, BLINK);
    lcd_status_line();
  }
}

#if defined(FRSKY) && defined(FRSKY_HUB) && defined(GPS) 
void displayGpsTime()
{
  uint8_t att = (TELEMETRY_STREAMING() ? LEFT|LEADING0 : LEFT|LEADING0|BLINK);
  lcdDrawNumberNAtt(CENTER_OFS+6*FW+7, STATUS_BAR_Y, frskyData.hub.hour, att, 2);
  lcdDrawCharAtt(CENTER_OFS+8*FW+4, STATUS_BAR_Y, ':', att);
  lcdDrawNumberNAtt(CENTER_OFS+9*FW+2, STATUS_BAR_Y, frskyData.hub.min, att, 2);
  lcdDrawCharAtt(CENTER_OFS+11*FW-1, STATUS_BAR_Y, ':', att);
  lcdDrawNumberNAtt(CENTER_OFS+12*FW-3, STATUS_BAR_Y, frskyData.hub.sec, att, 2);
  lcd_status_line();
}

void displayGpsCoord(uint8_t y, char direction, int16_t bp, int16_t ap)
{
  if (frskyData.hub.gpsFix >= 0) {
    if (!direction) direction = '-';
    lcdDrawNumberAttUnit(TELEM_2ND_COLUMN, y, bp / 100, LEFT); // ddd before '.'
    lcdDrawChar(lcdLastPos, y, '@');
    uint8_t mn = bp % 100; // TODO div_t
    if (g_eeGeneral.gpsFormat == 0) {
      lcdDrawChar(lcdLastPos+FWNUM, y, direction);
      lcdDrawNumberNAtt(lcdLastPos+FW+FW+1, y, mn, LEFT|LEADING0, 2); // mm before '.'
      lcdDrawSolidVerticalLine(lcdLastPos, y, 2);
      uint16_t ss = ap * 6;
      lcdDrawNumberNAtt(lcdLastPos+3, y, ss / 1000, LEFT|LEADING0, 2); // ''
      lcdDrawPoint(lcdLastPos, y+FH-2, 0); // small decimal point
      lcdDrawNumberNAtt(lcdLastPos+2, y, ss % 1000, LEFT|LEADING0, 3); // ''
      lcdDrawSolidVerticalLine(lcdLastPos, y, 2);
      lcdDrawSolidVerticalLine(lcdLastPos+2, y, 2);
    }
    else {
      lcdDrawNumberNAtt(lcdLastPos+FW, y, mn, LEFT|LEADING0, 2); // mm before '.'
      lcdDrawPoint(lcdLastPos, y+FH-2, 0); // small decimal point
      lcdDrawNumberNAtt(lcdLastPos+2, y, ap, LEFT|UNSIGN|LEADING0, 4); // after '.'
      lcdDrawChar(lcdLastPos+1, y, direction);
    }
  }
  else {
    // no fix
    lcdDrawText(TELEM_2ND_COLUMN, y, STR_VCSWFUNC+1/*----*/);
  }
}
#else
#define displayGpsTime()
#define displayGpsCoord(...)
#endif

NOINLINE uint8_t getRssiAlarmValue(uint8_t alarm)
{
  return (45 - 3*alarm + g_model.frsky.rssiAlarms[alarm].value);
}

void displayVoltageScreenLine(uint8_t y, uint8_t index)
{
  lcdDrawStringWithIndex(0, y, STR_A, index+1, 0);
  if (TELEMETRY_STREAMING()) {
    lcdPutsTelemetryChannelValue(3*FW+6*FW+4, y-FH, index+TELEM_A1-1, frskyData.analog[index].value, DBLSIZE);
    lcdDrawChar(12*FW-1, y-FH, '<'); lcdPutsTelemetryChannelValue(17*FW, y-FH, index+TELEM_A1-1, frskyData.analog[index].min, NO_UNIT);
    lcdDrawChar(12*FW, y, '>');      lcdPutsTelemetryChannelValue(17*FW, y, index+TELEM_A1-1, frskyData.analog[index].max, NO_UNIT);
  }
}

uint8_t barCoord(int16_t value, int16_t min, int16_t max)
{
  return limit((uint8_t)0, (uint8_t)(((int32_t)(BAR_WIDTH-1) * (value - min)) / (max - min)), (uint8_t)BAR_WIDTH);
}

void displayVoltagesScreen()
{
  // Volts / Amps / Watts / mAh
  uint8_t analog = 0;
  lcdDrawTextAtIndex(0, 2*FH, STR_AMPSRC, g_model.frsky.voltsSource+1, 0);
  switch (g_model.frsky.voltsSource) {
    case FRSKY_VOLTS_SOURCE_A1:
    case FRSKY_VOLTS_SOURCE_A2:
      displayVoltageScreenLine(2*FH, g_model.frsky.voltsSource);
      analog = 1+g_model.frsky.voltsSource;
      break;
#if defined(FRSKY_HUB)
    case FRSKY_VOLTS_SOURCE_FAS:
      lcdPutsTelemetryChannelValue(3*FW+6*FW+4, FH, TELEM_VFAS-1, frskyData.hub.vfas, DBLSIZE);
      break;
    case FRSKY_VOLTS_SOURCE_CELLS:
      lcdPutsTelemetryChannelValue(3*FW+6*FW+4, FH, TELEM_CELLS_SUM-1, frskyData.hub.cellsSum, DBLSIZE);
      break;
#endif
  }

  if (g_model.frsky.currentSource) {
    lcdDrawTextAtIndex(0, 4*FH, STR_AMPSRC, g_model.frsky.currentSource, 0);
    switch(g_model.frsky.currentSource) {
      case FRSKY_CURRENT_SOURCE_A1:
      case FRSKY_CURRENT_SOURCE_A2:
        displayVoltageScreenLine(4*FH, g_model.frsky.currentSource-1);
        break;
#if defined(FRSKY_HUB)
      case FRSKY_CURRENT_SOURCE_FAS:
        lcdPutsTelemetryChannelValue(3*FW+6*FW+4, 3*FH, TELEM_CURRENT-1, frskyData.hub.current, DBLSIZE);
        break;
#endif
    }

    lcdPutsTelemetryChannelValue(4, 5*FH, TELEM_POWER-1, frskyData.hub.power, LEFT|DBLSIZE);
    lcdPutsTelemetryChannelValue(3*FW+4+4*FW+6*FW+FW, 5*FH, TELEM_CONSUMPTION-1, frskyData.hub.currentConsumption, DBLSIZE);
  }
  else {
    displayVoltageScreenLine(analog > 0 ? 5*FH : 4*FH, analog ? 2-analog : 0);
    if (analog == 0) displayVoltageScreenLine(6*FH, 1);
  }

#if defined(FRSKY_HUB)
  // Cells voltage
  if (frskyData.hub.cellsCount > 0) {
    uint8_t y = 1*FH;
    for (uint8_t k=0; k<frskyData.hub.cellsCount && k<6; k++) {
#if defined(GAUGES)
      uint8_t attr = (barsThresholds[THLD_CELL] && frskyData.hub.cellVolts[k] < barsThresholds[THLD_CELL]) ? BLINK|PREC2 : PREC2;
#else
      uint8_t attr = PREC2;
#endif
      lcdDrawNumberNAtt(LCD_W, y, TELEMETRY_CELL_VOLTAGE(k), attr, 4);
      y += 1*FH;
    }
    lcdDrawSolidVerticalLine(LCD_W-3*FW-2, 8, 47);
  }
#endif

  displayRssiLine();
}

void displayAfterFlightScreen()
{
  uint8_t line=1*FH+1;
  if (IS_GPS_AVAILABLE()) {
    // Latitude
    lcdDrawTextLeft(line, STR_LATITUDE);
    displayGpsCoord(line, frskyData.hub.gpsLatitudeNS, frskyData.hub.gpsLatitude_bp, frskyData.hub.gpsLatitude_ap);
    // Longitude
    line+=1*FH+1;
    lcdDrawTextLeft(line, STR_LONGITUDE);
    displayGpsCoord(line, frskyData.hub.gpsLongitudeEW, frskyData.hub.gpsLongitude_bp, frskyData.hub.gpsLongitude_ap);
    displayGpsTime();
    line+=1*FH+1;
  }
  // Rssi
  lcdDrawTextLeft(line, STR_MINRSSI);
  lcdDrawText(TELEM_2ND_COLUMN, line, STR_TX);
  lcdDrawNumberNAtt(TELEM_2ND_COLUMN+3*FW, line, frskyData.rssi[1].min, LEFT|LEADING0, 2);
  lcdDrawText(TELEM_2ND_COLUMN+6*FW, line, STR_RX);
  lcdDrawNumberNAtt(TELEM_2ND_COLUMN+9*FW, line, frskyData.rssi[0].min, LEFT|LEADING0, 2);
}

bool displayGaugesTelemetryScreen(FrSkyScreenData & screen)
{
  // Custom Screen with gauges
  uint8_t barHeight = 5;
  for (int8_t i=3; i>=0; i--) {
    FrSkyBarData & bar = screen.bars[i];
    source_t source = bar.source;
    getvalue_t barMin = convertBarTelemValue(source, bar.barMin);
    getvalue_t barMax = convertBarTelemValue(source, 255-bar.barMax);
    if (source && barMax > barMin) {
      uint8_t y = barHeight+6+i*(barHeight+6);
      lcdDrawTextAtIndex(0, y+barHeight-5, STR_VTELEMCHNS, source, 0);
      lcdDrawRect(BAR_LEFT, y, BAR_WIDTH+1, barHeight+2);
      getvalue_t value = getValue(MIXSRC_FIRST_TELEM+source-1);

      uint8_t thresholdX = 0;

      getvalue_t threshold = 0;
      if (source <= TELEM_TIMER_MAX)
        threshold = 0;
      else if (source <= TELEM_RSSI_RX)
        threshold = getRssiAlarmValue(source-TELEM_RSSI_TX);
      else if (source <= TELEM_A2)
        threshold = g_model.frsky.channels[source-TELEM_A1].alarms_value[0];
#if defined(FRSKY_HUB)
      else {
#if defined(GAUGES)
        threshold = convertBarTelemValue(source, barsThresholds[source-TELEM_ALT]);
#endif
      }
#endif

      if (threshold) {
        thresholdX = barCoord(threshold, barMin, barMax);
        if (thresholdX == 100)
          thresholdX = 0;
      }

      uint8_t width = barCoord(value, barMin, barMax);

      // reversed barshade for T1/T2
      uint8_t barShade = ((threshold > value) ? DOTTED : SOLID);
      if (source == TELEM_T1 || source == TELEM_T2) {
        barShade = -barShade;
      }

      lcdDrawFilledRect(BAR_LEFT+1, y+1, width, barHeight, barShade);

      for (uint8_t j=24; j<99; j+=25) {
        if (j>thresholdX || j>width) {
          lcdDrawSolidVerticalLine(j*BAR_WIDTH/100+BAR_LEFT+1, y+1, barHeight);
        }
      }

      if (thresholdX) {
        lcdDrawSolidVerticalLineStip(BAR_LEFT+1+thresholdX, y-2, barHeight+3, DOTTED);
        lcdDrawSolidHorizontalLine(BAR_LEFT+thresholdX, y-2, 3);
      }
    }
    else {
      barHeight += 2;
    }
  }
  displayRssiLine();
  return barHeight < 13;
}

bool displayNumbersTelemetryScreen(FrSkyScreenData & screen)
{
  // Custom Screen with numbers
  uint8_t fields_count = 0;
  for (uint8_t i=0; i<4; i++) {
    for (uint8_t j=0; j<NUM_LINE_ITEMS; j++) {
      uint8_t field = screen.lines[i].sources[j];
      if (field > 0) {
        fields_count++;
      }
      if (i==3) {
        lcdDrawSolidVerticalLine(63, 8, 48);
        if (TELEMETRY_STREAMING()) {
#if defined(FRSKY_HUB)
          if (field == TELEM_ACC) {
            lcdDrawTextLeft(STATUS_BAR_Y, STR_ACCEL);
            lcdDrawNumberNAtt(4*FW, STATUS_BAR_Y, frskyData.hub.accelX, LEFT|PREC2);
            lcdDrawNumberNAtt(10*FW, STATUS_BAR_Y, frskyData.hub.accelY, LEFT|PREC2);
            lcdDrawNumberNAtt(16*FW, STATUS_BAR_Y, frskyData.hub.accelZ, LEFT|PREC2);
            break;
          }
#endif
#if defined(FRSKY_HUB) && defined(GPS)
          else if (field == TELEM_GPS_TIME) {
            displayGpsTime();
            break;
          }
#endif
        }
        else {
          displayRssiLine();
          return fields_count;
        }
      }
      if (field) {
        getvalue_t value = getValue(MIXSRC_FIRST_TELEM+field-1);
        uint8_t att = (i==3 ? NO_UNIT : DBLSIZE|NO_UNIT);
        coord_t pos[] = {0, 65, 130};
        lcdPutsTelemetryChannelValue(pos[j+1]-2, FH+2*FH*i, field-1, value, att);

        if (field >= TELEM_TIMER1 && field <= TELEM_TIMER_MAX && i!=3) {
          // there is not enough space on LCD for displaying "Tmr1" or "Tmr2" and still see the - sign, we write "T1" or "T2" instead
          field = field-TELEM_TIMER1+TELEM_T1;
        }

        lcdDrawTextAtIndex(pos[j], 1+FH+2*FH*i, STR_VTELEMCHNS, field, 0);
      }
    }
  }
  lcd_status_line();
  return fields_count;
}

bool displayCustomTelemetryScreen(uint8_t index)
{
  FrSkyScreenData & screen = g_model.frsky.screens[index];

#if defined(GAUGES)
  if (IS_BARS_SCREEN(s_frsky_view)) {
    return displayGaugesTelemetryScreen(screen);
  }
#endif

  return displayNumbersTelemetryScreen(screen);
}

bool displayTelemetryScreen()
{

  lcdDrawTelemetryTopBar();

  if (s_frsky_view < MAX_TELEMETRY_SCREENS) {
    return displayCustomTelemetryScreen(s_frsky_view);
  }

  if (s_frsky_view == TELEMETRY_VOLTAGES_SCREEN) {
    displayVoltagesScreen();
  }

#if defined(FRSKY_HUB)
  else {
    displayAfterFlightScreen();
  }
#endif

  return true;
}

void decrTelemetryScreen()
{
  if (s_frsky_view-- == 0)
    s_frsky_view = TELEMETRY_VIEW_MAX;
}
void incrTelemetryScreen()
{
  if (s_frsky_view++ == TELEMETRY_VIEW_MAX)
    s_frsky_view = 0;
}

void menuTelemetryFrsky(uint8_t event)
{

  switch (event) {
    case EVT_KEY_FIRST(KEY_EXIT):
      killEvents(event);
      chainMenu(menuMainView);
      break;

    case EVT_KEY_FIRST(KEY_UP):
      decrTelemetryScreen();
      break;

    case EVT_KEY_FIRST(KEY_DOWN):
      incrTelemetryScreen();
      break;

    case EVT_KEY_FIRST(KEY_ENTER):
      telemetryReset();
      break;
  }

  if (!displayTelemetryScreen()) {
    putEvent(event == EVT_KEY_FIRST(KEY_UP) ? event : EVT_KEY_FIRST(KEY_DOWN));
  }
}

