/*
    Copyright 2017 Ivan Gorinov

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */

#include <string.h>

#include <LiquidCrystal.h>

#define SCL_PORT PORTD
#define SCL_PIN 2
#define SDA_PORT PORTD
#define SDA_PIN 3
#define I2C_TIMEOUT 1000
#define I2C_SLOWMODE 1

#include <SoftI2CMaster.h>

byte address = 0x80;
char str_rh[16];
char str_tc[16];
char str_tf[16];

char sensor_name[] = "Si70xx";
char digit[17] = "0123456789ABCDEF";
byte sna[4];
byte snb[4];

const int rs = 8, en = 9;
LiquidCrystal lcd(rs, en, 4, 5, 6, 7);

int si70xx_read_id(void)
{
  byte crc;
  byte i;

  if (!i2c_start(address))
    return 0;

  /* Read Electronic ID 1st Byte */

  i2c_write(0xFA);
  i2c_write(0x0F);

  if (!i2c_rep_start(address | 1))
    return 0;

  for (i = 0; i < 4; i += 1) {
    sna[3 - i] = i2c_read(false);
    crc = i2c_read(false);
  }

  i2c_stop();

  if (!i2c_start(address))
    return 0;

  /* Read Electronic ID 2nd Byte */

  i2c_write(0xFC);
  i2c_write(0xC9);

  if (!i2c_rep_start(address | 1))
    return 0;

  for (i = 0; i < 4; i += 1) {
    snb[3 - i] = i2c_read(false);
    crc = i2c_read(false);
  }

  i2c_stop();
}

void setup()
{
  Serial.begin(9600);

  pinMode(SCL_PIN, INPUT_PULLUP);
  pinMode(SDA_PIN, INPUT_PULLUP);

  i2c_init();

  /* Set number of columns and rows */
  lcd.begin(16, 2);

  if (si70xx_read_id()) {
    if (snb[3] < 100) {
      sensor_name[4] = digit[snb[3] / 10];
      sensor_name[5] = digit[snb[3] % 10];
    }
    lcd.print(sensor_name);
  }
}

void loop()
{
  byte status = 0;
  long d = 0;
  long t, tf;
  word rh = 0;
  byte sum;
  char *p;
  byte c, i;

  delay(3000);

  if (!i2c_start(address))
    return;

  /* Measure Relative Humidity, Hold Master Mode */

  i2c_write(0xE5);

  if (!i2c_rep_start(address | 1))
    return;

  d = i2c_read(false);
  d <<= 8;
  d |= i2c_read(true);
  i2c_stop();
  
  rh = ((d * 125) >> 16) - 6;

  if (!i2c_rep_start(address))
    return;

  /* Read Temperature Value from Previous RH Measurement */

  i2c_write(0xE0);

  if (!i2c_rep_start(address | 1))
    return;

  d = i2c_read(false);
  d <<= 8;
  d |= i2c_read(true);
  i2c_stop();
  
  t = ((d * 17572) >> 16) - 4685;
  tf = (t * 9 / 5) + 3200;
  t = (t + 5) / 10;
  tf = (tf + 5) / 10;

  strcpy(str_rh, "RH:   %");
  i = 5;
  do {
    str_rh[i--] = digit[rh % 10];
    rh /= 10;
  } while (rh != 0);

  strcpy(str_tc, "   .  C");
  i = 4;
  str_tc[i--] = digit[t % 10];
  t /= 10;
  str_tc[i--] = '.';
  do {
    str_tc[i--] = digit[t % 10];
    t /= 10;
  } while (t != 0);
  

  strcpy(str_tf, "   .  F");
  i = 4;
  str_tf[i--] = digit[tf % 10];
  tf /= 10;
  str_tf[i--] = '.';
  do {
    str_tf[i--] = digit[tf % 10];
    tf /= 10;
  } while (tf != 0);
   
  Serial.println(str_rh);
  lcd.setCursor(0, 0);
  lcd.print(str_rh);
  lcd.setCursor(8, 0);
  lcd.print(str_tc);
  lcd.setCursor(8, 1);
  lcd.print(str_tf);
}

