/*
  This is responsible for all time related methods

  Call configNtpTime() in your setup()

  If needed, callin setup(): activateNtpTimeUpdateNotification();
*/

#include <time.h>
#include "sntp.h" // ntp time update notification

// Timezone and NTP servers
const char *ntpServer = "pool.ntp.org";

// choose your time zone from this list
// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define MY_TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"

/*
To use a 24 hour display, set this line const bool MilTime = true; to true. 
To use a 12 hour display, set MilTime to false, const bool MilTime = false;
*/
const bool MilTime = true;  // 24 hour clock, AKA military time

// Time Globals
time_t now;  // this are the seconds since Epoch (1970) - UTC
tm tm;       // the structure tm holds time information in a more convenient way *
uint8_t hh, mm, ss;
uint8_t dy, mt;  // day and month
uint16_t yr;     // year

int8_t currentMinute1 = 9;
int8_t currentMinute2 = 9;
int8_t currentHour1 = 9;
int8_t currentHour2 = 9;
int8_t currentDate1 = 9;
int8_t currentDate2 = 9;
char currentDOW[4] = "XXX";
char currentMonth[4] = "XXX";
bool firstpass;

const char *monthsOfYear[] = {
  "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
  "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

const char *daysOfWeek[] = {
  "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};

void getCurrentTime() {
  time(&now);              // read the current time
  localtime_r(&now, &tm);  // update the structure tm with the current time
  hh = tm.tm_hour;
  mm = tm.tm_min;
  ss = tm.tm_sec;
  dy = tm.tm_mday;
  mt = tm.tm_mon + 1;
  yr = tm.tm_year + 1900;
}

void printCurrentTime() {
  time(&now);              // read the current time
  localtime_r(&now, &tm);  // update the structure tm with the current time
/*
  Serial.print("year:");
  Serial.print(tm.tm_year + 1900);  // years since 1900
  Serial.print("\tmonth:");
  Serial.print(tm.tm_mon + 1);  // January = 0 (!)
  Serial.print("\tday:");
  Serial.print(tm.tm_mday);  // day of month
  Serial.print("\thour:");
  Serial.print(tm.tm_hour);  // hours since midnight 0-23
  Serial.print("\tmin:");
  Serial.print(tm.tm_min);  // minutes after the hour 0-59
  Serial.print("\tsec:");
  Serial.print(tm.tm_sec);  // seconds after the minute 0-61*
  Serial.print("\twday");
  Serial.print(tm.tm_wday);  // days since Sunday 0-6
*/
  Serial.printf("Day %02d month %02d year %d hour %02d min %02d sec %02d ", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
  if (tm.tm_isdst == 1)      // Daylight Saving Time flag
    Serial.println("Daylight Saving Time");
  else
    Serial.println("Standard Time");
}

// Callback function (gets called when time adjusts via NTP) 
//triggered by sntp_set_time_sync_notification_cb( timeavailable );
void ntpTimeUpdateAvailable(struct timeval *t)
{
  Serial.println("Got time adjustment from NTP!");
  //Put any code in here that you want executed when NTP updates
  //e.g.
  //printLocalTime();
  printCurrentTime();
}

void activateNtpTimeUpdateNotification() {
  // set notification call-back function
  sntp_set_time_sync_notification_cb(ntpTimeUpdateAvailable); // tiggers function timeavailable when SNTP update has taken place

}

void update_clock(bool force_update) {
  time_t now = time(NULL);  // Get current time as time_t
  struct tm timeInfo;
  localtime_r(&now, &timeInfo);  // Convert time_t to struct tm

  int nowDOWNumber = timeInfo.tm_wday;  // Get day of week (0 = Sunday, 6 = Saturday)
  int hours = timeInfo.tm_hour;
  int nowH1;
  int nowH2;
  if (!MilTime && (hours > 12)) {
    hours = hours - 12;
    nowH1 = hours / 10;  // First hour digit
    nowH2 = hours % 10;  // Second hour digit
  } else {
    nowH1 = hours / 10;  // First hour digit
    nowH2 = hours % 10;  // Second hour digit
  }

  int nowM1 = timeInfo.tm_min / 10;  // First minute digit
  int nowM2 = timeInfo.tm_min % 10;  // Second minute digit
  int nowMonthNumber = timeInfo.tm_mon;
  int nowDate1 = timeInfo.tm_mday / 10;
  int nowDate2 = timeInfo.tm_mday % 10;

  char nowMonthCharacter[2];
  nowMonthCharacter[0] = monthsOfYear[nowMonthNumber][0];
  nowMonthCharacter[1] = '\0';

  char placeholderNow[2];
  char placeholderCurrent[2];

  char DOWnowCharacter[2];
  DOWnowCharacter[0] = daysOfWeek[nowDOWNumber][0];
  DOWnowCharacter[1] = '\0';

  if ((currentMinute2 != nowM2) || force_update) {
    snprintf(placeholderNow, sizeof(placeholderNow), "%c", nowM2 + '0');
    snprintf(placeholderCurrent, sizeof(placeholderCurrent), "%c", currentMinute2 + '0');
    //change_sprite_display( placeholderCurrent, placeholderNow, m2X, largeSpriteTop, true );
    currentMinute2 = nowM2;
  }

  if ((currentMinute1 != nowM1) || force_update) {
    snprintf(placeholderNow, sizeof(placeholderNow), "%c", nowM1 + '0');
    snprintf(placeholderCurrent, sizeof(placeholderCurrent), "%c", currentMinute1 + '0');
    //change_sprite_display( placeholderCurrent, placeholderNow, m1X, largeSpriteTop, true );
    currentMinute1 = nowM1;
  }
  if ((currentHour2 != nowH2) || force_update) {
    snprintf(placeholderNow, sizeof(placeholderNow), "%c", nowH2 + '0');
    snprintf(placeholderCurrent, sizeof(placeholderCurrent), "%c", currentHour2 + '0');
    //change_sprite_display( placeholderCurrent, placeholderNow, h2X, largeSpriteTop, true );
    currentHour2 = nowH2;
  }

  if ((currentHour1 != nowH1) || force_update) {
    snprintf(placeholderNow, sizeof(placeholderNow), "%c", nowH1 + '0');
    snprintf(placeholderCurrent, sizeof(placeholderCurrent), "%c", currentHour1 + '0');
    //change_sprite_display( placeholderCurrent, placeholderNow, h1X, largeSpriteTop, true );
    currentHour1 = nowH1;
    if ((nowH2 == 0) || force_update) {  // it is a new day
      Serial.printf("Current day of week is %s to start\n", currentDOW);
      Serial.printf("Current Month  is %s to start\n", currentMonth);
      snprintf(placeholderCurrent, sizeof(placeholderCurrent), "%c", currentDOW[0]);
      DOWnowCharacter[0] = daysOfWeek[nowDOWNumber][0];
      //change_sprite_display( placeholderCurrent, DOWnowCharacter, DOW1X, smallSpriteTop, false );

      snprintf(placeholderCurrent, sizeof(placeholderCurrent), "%c", currentDOW[1]);
      DOWnowCharacter[0] = daysOfWeek[nowDOWNumber][1];
      //change_sprite_display( placeholderCurrent, DOWnowCharacter, DOW2X, smallSpriteTop, false );

      snprintf(placeholderCurrent, sizeof(placeholderCurrent), "%c", currentDOW[2]);
      DOWnowCharacter[0] = daysOfWeek[nowDOWNumber][2];
      //change_sprite_display( placeholderCurrent, DOWnowCharacter, DOW3X, smallSpriteTop, false );

      strcpy(currentDOW, &daysOfWeek[nowDOWNumber][0]);  // we get everything until the first null character

      snprintf(placeholderNow, sizeof(placeholderNow), "%c", nowDate1 + '0');
      snprintf(placeholderCurrent, sizeof(placeholderCurrent), "%c", currentDate1 + '0');
      //change_sprite_display( placeholderCurrent, placeholderNow, Date1X, smallSpriteTop, false );

      snprintf(placeholderNow, sizeof(placeholderNow), "%c", nowDate2 + '0');              // conv int to character
      snprintf(placeholderCurrent, sizeof(placeholderCurrent), "%c", currentDate2 + '0');  // conv int to character
      //change_sprite_display( placeholderCurrent, placeholderNow, Date2X, smallSpriteTop, false );

      currentDate1 = nowDate1;
      currentDate2 = nowDate2;

      snprintf(placeholderCurrent, sizeof(placeholderCurrent), "%c", currentMonth[0]);
      nowMonthCharacter[0] = monthsOfYear[nowMonthNumber][0];
      //change_sprite_display( placeholderCurrent, nowMonthCharacter, Month1X, smallSpriteTop, false );

      snprintf(placeholderCurrent, sizeof(placeholderCurrent), "%c", currentMonth[1]);
      nowMonthCharacter[0] = monthsOfYear[nowMonthNumber][1];
      //change_sprite_display( placeholderCurrent, nowMonthCharacter, Month2X, smallSpriteTop, false );

      snprintf(placeholderCurrent, sizeof(placeholderCurrent), "%c", currentMonth[2]);
      nowMonthCharacter[0] = monthsOfYear[nowMonthNumber][2];
      //change_sprite_display( placeholderCurrent, nowMonthCharacter, Month3X, smallSpriteTop, false );

      strcpy(currentMonth, &monthsOfYear[nowMonthNumber][0]);
      Serial.printf("Current DOW is %s to end\n", currentDOW);
      Serial.printf("Current Month  is %s to end\n", currentMonth);
    }
  }
}

void configNtpTime() {
  // Obtain current time and set variables for the Second, Minute and Hour
  configTime(0, 0, ntpServer);   // 0, 0 because we will use TZ in the next line
  setenv("TZ", MY_TIMEZONE, 1);  // Set environment variable with your time zone
  tzset();
}