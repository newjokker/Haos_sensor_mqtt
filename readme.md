# 说明

### 环境

```bash

platformio lib install "Adafruit GFX Library"

platformio lib install "Adafruit SSD1306"

platformio lib install "DHT sensor library"

platformio lib install "PubSubClient"

pio lib install "Adafruit NeoPixel"

pio pkg install --library 1076

pio pkg install --library "knolleary/PubSubClient"

pio lib install "Stepper"

pio lib install "AccelStepper"

```

### 调试代码

* 串口调试
```bash

tio -b 115200 --timestamp  /dev/cu.wchusbserial5A7B1617701 

```

* python 代码调试

```bash

python3 ./tools/read_csv.py /dev/cu.wchusbserial5A7B1617701 

```


### 需求

* 能实现上传一个或者多个实体，查看用什么方案

* 

### 已经解决

* 当前使用两个板子同时通电的时候两块板子互相影响，不知道是哪里出现了问题 （应该是使用了同一个 client_id 导致的）



