/*
 * BaroHelper.cpp
 * Copyright (C) 2018-2021 Linar Yusupov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../system/SoC.h"

#include "Baro.h"

#if defined(EXCLUDE_BMP180) && defined(EXCLUDE_BMP280) && defined(EXCLUDE_MPL3115A2) && defined(EXCLUDE_MS5611)
byte Baro_setup()    {return BARO_MODULE_NONE;}
void Baro_loop()     {}
#else

#if !defined(EXCLUDE_BMP180)
#include <Adafruit_BMP085.h>
#endif /* EXCLUDE_BMP180 */
#if !defined(EXCLUDE_BMP280)
#include <Adafruit_BMP280.h>
#endif /* EXCLUDE_BMP280 */
#if !defined(EXCLUDE_MPL3115A2)
#include <Adafruit_MPL3115A2.h>
#endif /* EXCLUDE_MPL3115A2 */
#if !defined(EXCLUDE_MS5611)
#include <MS5x.h>
#endif /* EXCLUDE_MS5611 */

#include <TinyGPS++.h>

barochip_ops_t *baro_chip = NULL;

#if !defined(EXCLUDE_BMP180)
Adafruit_BMP085 bmp180;
#endif /* EXCLUDE_BMP180 */
#if !defined(EXCLUDE_BMP280)
Adafruit_BMP280 bmp280;
#endif /* EXCLUDE_BMP280 */
#if !defined(EXCLUDE_MPL3115A2)
Adafruit_MPL3115A2 mpl3115a2 = Adafruit_MPL3115A2();
#endif /* EXCLUDE_MPL3115A2 */
#if !defined(EXCLUDE_MS5611)
MS5x ms5611(&Wire);
#endif /* EXCLUDE_MS5611 */

static unsigned long BaroTimeMarker = 0;
static float prev_pressure_altitude = 0;

// increasing the averaging factor to average more data from sensor form 4 to 10
#define VS_AVERAGING_FACTOR   10
static float Baro_VS[VS_AVERAGING_FACTOR];
static int avg_ndx = 0;

/* 4 baro sensor readings per second */
#define isTimeToBaro() ((millis() - BaroTimeMarker) > (1000 / VS_AVERAGING_FACTOR))

#if !defined(EXCLUDE_BMP180)
static bool bmp180_probe()
{
  return bmp180.begin();
}

static void bmp180_setup()
{
  Serial.print(F("Temperature = "));
  Serial.print(bmp180.readTemperature());
  Serial.println(F(" *C"));
  
  Serial.print(F("Pressure = "));
  Serial.print(bmp180.readPressure());
  Serial.println(F(" Pa"));
  
  // Calculate altitude assuming 'standard' barometric
  // pressure of 1013.25 millibar = 101325 Pascal
  Serial.print(F("Altitude = "));
  Serial.print(bmp180.readAltitude());
  Serial.println(F(" meters"));

  Serial.print(F("Pressure at sealevel (calculated) = "));
  Serial.print(bmp180.readSealevelPressure());
  Serial.println(F(" Pa"));

// you can get a more precise measurement of altitude
// if you know the current sea level pressure which will
// vary with weather and such. If it is 1015 millibars
// that is equal to 101500 Pascals.
  Serial.print(F("Real altitude = "));
  Serial.print(bmp180.readAltitude(101500));
  Serial.println(F(" meters"));
  
  Serial.println();
  delay(500);
}

static float bmp180_altitude(float sealevelPressure)
{
  return bmp180.readAltitude(sealevelPressure * 100);
}

barochip_ops_t bmp180_ops = {
  BARO_MODULE_BMP180,
  "BMP180",
  bmp180_probe,
  bmp180_setup,
  bmp180_altitude
};
#endif /* EXCLUDE_BMP180 */

#if !defined(EXCLUDE_BMP280)
static bool bmp280_probe()
{
  return (
          bmp280.begin(BMP280_ADDRESS,     BMP280_CHIPID) ||
          bmp280.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID) ||
          bmp280.begin(BMP280_ADDRESS,     BME280_CHIPID) ||
          bmp280.begin(BMP280_ADDRESS_ALT, BME280_CHIPID)
         );
}

static void bmp280_setup()
{
    Serial.print(F("Temperature = "));
    Serial.print(bmp280.readTemperature());
    Serial.println(F(" *C"));
    
    Serial.print(F("Pressure = "));
    Serial.print(bmp280.readPressure());
    Serial.println(F(" Pa"));

    Serial.print(F("Approx altitude = "));
    Serial.print(bmp280.readAltitude(1013.25)); // this should be adjusted to your local forcase
    Serial.println(F(" m"));
    
    Serial.println();
    delay(500);
}

static float bmp280_altitude(float sealevelPressure)
{
    return bmp280.readAltitude(sealevelPressure);
}

barochip_ops_t bmp280_ops = {
  BARO_MODULE_BMP280,
  "BMP280",
  bmp280_probe,
  bmp280_setup,
  bmp280_altitude
};
#endif /* EXCLUDE_BMP280 */

#if !defined(EXCLUDE_MPL3115A2)
static bool mpl3115a2_probe()
{
  return mpl3115a2.begin();
}

static void mpl3115a2_setup()
{
  float pascals = mpl3115a2.getPressure();
  // Our weather page presents pressure in Inches (Hg)
  // Use http://www.onlineconversion.com/pressure.htm for other units
  Serial.print(pascals/3377); Serial.println(F(" Inches (Hg)"));

  float altm = mpl3115a2.getAltitude();
  Serial.print(altm); Serial.println(F(" meters"));

  float tempC = mpl3115a2.getTemperature();
  Serial.print(tempC); Serial.println(F("*C"));

  delay(250);
}

static float mpl3115a2_altitude(float sealevelPressure)
{
  mpl3115a2.setSeaPressure(sealevelPressure * 100);
  return mpl3115a2.getAltitude();
}

barochip_ops_t mpl3115a2_ops = {
  BARO_MODULE_MPL3115A2,
  "MPL3115A2",
  mpl3115a2_probe,
  mpl3115a2_setup,
  mpl3115a2_altitude
};
#endif /* EXCLUDE_MPL3115A2 */

#if !defined(EXCLUDE_MS5611)
		
double prevPressure=0; // The value of the pressure the last time the sensor was polled
double prevTemperature=0; // The value of the temperature the last time the sensor was polled
double sealevelPressure = 0;
		
static bool ms5611_probe()
{
  // Definition of first I2C bus (pins 13 ,2 on T-Beam ) where baro sensors are connected is done in ESP32.cpp file on line 1101
  delay(500); //Let sensor start and initialize
  Serial.print(F("Detecting MS5611 baro sensor"));
  Serial.println();
  
  // Keep in mind that by default the library sets ic2address to default of 0x77. Controlled by I2C_HIGH or I2C_LOW variables in the lib itself
  while (ms5611.connect()>0) { // ms5611.connect starts wire and attempts to connect to sensor
		Serial.println(F("MS5611 baro sensor not found"));
		delay(500);
		return false;
  }
	Serial.println(F("Connected to MS5611 Sensor"));
	ms5611.setDelay(1000); // barometer will wait 250 ms before taking new temperature and pressure readings
	return true;
}

static void ms5611_setup()
{
		//Serial.print(F("Begin MS5611 setup function "));
		//Serial.println();
}

static float ms5611_altitude(float TempsealevelPressure)
{
  double pressure = 0;
  double temperature = 0;
  double altitude = 0;

  ms5611.checkUpdates();

  if (ms5611.isReady()) { 	
		temperature = ms5611.GetTemp(); // Returns temperature in C
		pressure = ms5611.GetPres(); // Returns pressure in Pascals
		
		// If a new temperature or pressure reading comes in then the sensor has been polled.
		// Print the new values and the time, in ms, between readings
		if ((temperature != prevTemperature) || (pressure != prevPressure)) {
			
			// Calculate predicted seaLevel pressure based off a known altitude in meters
			if (sealevelPressure == 0) sealevelPressure = ms5611.getSeaLevel(217.3); 
/*
			Serial.print(F("The current pressure is: "));
			Serial.print(pressure);
			Serial.println(F(" Pascals"));

			Serial.print(F("The current temperature is: "));
			Serial.print(temperature);
			Serial.println(F("°C"));
		
			Serial.print(F("The calculated pressure at seaLevel is: "));
			Serial.print(sealevelPressure);
			Serial.println(F(" Pascals"));

			// Calculate current altitude based on pressure reading
			altitude = ms5611.getAltitude();
			Serial.print(F("The calculated altitude is: "));
			Serial.print(altitude);
			Serial.println(F(" meters"));
			
			// Calculate current altitude performing temperature corrections
			altitude = ms5611.getAltitude(true);
			Serial.print(F("The calculated altitude with temperature correction is: "));
			Serial.print(altitude);
			Serial.println(F(" meters"));
			
			Serial.println();
			Serial.println();
*/
			prevTemperature = temperature;
			prevPressure = pressure;
		}
	} 
	return ms5611.getAltitude();
}

barochip_ops_t ms5611_ops = {
  BARO_MODULE_MS5611,
  "MS5611",
  ms5611_probe,
  ms5611_setup,
  ms5611_altitude
};
#endif /* EXCLUDE_MS5611 */

bool Baro_probe()
{
  return (
#if !defined(EXCLUDE_BMP180)
           (baro_chip = &bmp180_ops,    baro_chip->probe()) ||
#else
           false                                            ||
#endif /* EXCLUDE_BMP180 */

#if !defined(EXCLUDE_BMP280)
           (baro_chip = &bmp280_ops,    baro_chip->probe()) ||
#else
           false                                            ||
#endif /* EXCLUDE_BMP280 */

#if !defined(EXCLUDE_MS5611)
           (baro_chip = &ms5611_ops,    baro_chip->probe()) ||
#else
           false                                            ||
#endif /* EXCLUDE_MS5611 */

#if !defined(EXCLUDE_MPL3115A2)
           (baro_chip = &mpl3115a2_ops, baro_chip->probe())
#else
           false
#endif /* EXCLUDE_MPL3115A2 */
         );
}

byte Baro_setup()
{
  if ( SoC->Baro_setup() && Baro_probe() ) {

    Serial.print(baro_chip->name);
    Serial.println(F(" barometric pressure sensor is detected."));

    baro_chip->setup();

    prev_pressure_altitude = baro_chip->altitude(1013.25);
    BaroTimeMarker = millis();

    for (int i=0; i<VS_AVERAGING_FACTOR; i++) {
      Baro_VS[i] = 0;
    }

    return baro_chip->type;

  } else {
    baro_chip = NULL;
    Serial.println(F("WARNING! Barometric pressure sensor is NOT detected."));

    return BARO_MODULE_NONE;
  }
}

void Baro_loop()
{
  if (baro_chip && isTimeToBaro()) {

    /* Draft of pressure altitude and vertical speed calculation */
    ThisAircraft.pressure_altitude = baro_chip->altitude(1013.25);

    Baro_VS[avg_ndx] = (ThisAircraft.pressure_altitude - prev_pressure_altitude) /
      (millis() - BaroTimeMarker) * 1000;  /* in m/s */

    ThisAircraft.vs = 0;
    for (int i=0; i<VS_AVERAGING_FACTOR; i++) {
      ThisAircraft.vs += Baro_VS[i];
    }
    ThisAircraft.vs /= VS_AVERAGING_FACTOR;

    if (ThisAircraft.vs > -0.1 && ThisAircraft.vs < 0.1) {
      ThisAircraft.vs = 0;
    }

    ThisAircraft.vs *= (_GPS_FEET_PER_METER * 60.0) ; /* feet per minute */

    prev_pressure_altitude = ThisAircraft.pressure_altitude;
    BaroTimeMarker = millis();
    avg_ndx = (avg_ndx + 1) % VS_AVERAGING_FACTOR;

#if 0
    Serial.print(F("P.Alt. = ")); Serial.print(ThisAircraft.pressure_altitude);
    Serial.print(F(" , VS avg. = ")); Serial.println(ThisAircraft.vs);
#endif
  }
}

#endif /* EXCLUDE_BMP180 && EXCLUDE_BMP280 && EXCLUDE_MPL3115A2 && EXCLUDE_MS5611 */
