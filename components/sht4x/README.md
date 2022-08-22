# SHT4x

- This component will show you how to use I2C module read external i2c sensor data, here we use SHT3x-series temperature and humidity sensor(SHT30 is used this component).
- Pin assignment:
  - GPIO21 is assigned as the data signal of i2c master port
  - GPIO22 is assigned as the clock signal of i2c master port
- Connection:
   * connect sda of sensor with GPIO21 
   * connect scl of sensor with GPIO22
- SHT4x measurement mode:
  * single shot data acquisition mode: in this mode one issued measurement command triggers the acquisition of one data pair.
