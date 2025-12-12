# 📷 STM32F103 ArduCAM OV5642 Video Streamer

**STM32F103RB** 마이크로컨트롤러와 **ArduCAM OV5642** 모듈을 활용한 고속 JPEG 비디오 스트리밍 시스템입니다.  
제한된 MCU 자원 환경에서도 하드웨어 JPEG 압축과 **UART 통신 최적화(Burst Read)**를 통해 부드러운 실시간 영상을 제공하는 것을 목표로 합니다.

---

## 📖 프로젝트 개요 (Overview)
이 프로젝트는 임베디드 환경에서 카메라 데이터를 효율적으로 처리하는 전체 파이프라인을 구현합니다. STM32의 SPI 인터페이스로 카메라 데이터를 고속으로 읽어들이고, UART(Serial) 인터페이스를 통해 PC로 전송하여 실시간 모니터링을 수행합니다.

---

## ✨ 주요 기능 (Key Features)

### ⚡ 실시간 스트리밍 (Real-time Streaming)
*   끊김 없는 MJPEG 비디오 전송 (약 8~10 FPS 달성).
*   Burst Read 모드와 고속 Baud Rate(921600) 적용으로 대역폭 최적화.

### 🚀 하드웨어 가속 (Hardware Acceleration)
*   OV5642 내장 JPEG 엔진을 활용하여 MCU의 연산 부하 최소화 및 데이터 크기 압축.

### 🎨 스마트 화질 제어 (Smart Image Control)
*   **Auto Exposure**: 조도에 따른 자동 노출 및 감도 조절.
*   **Anti-Banding**: 형광등 및 LED 조명의 플리커(깜빡임) 현상 제거.
*   **Timing Tuning**: PCLK 및 VSYNC 정밀 튜닝으로 노이즈 없는 깨끗한 영상 확보.

---

## 🛠 하드웨어 구성 (Hardware Setup)

### 필수 부품 (Components)
*   **MCU**: STM32 Nucleo-F103RB (또는 호환 보드)
*   **Camera**: ArduCAM Mini Module Camera Shield w/ 5MP OV5642
*   **Etc**: 점퍼 케이블 (Female-to-Female)

### 🔌 핀 연결도 (Pin Map)
STM32F103 보드와 ArduCAM 모듈을 아래 표에 맞춰 연결합니다.

| STM32 Pin | ArduCAM Pin | 기능 (Function) | 비고 |
| :---: | :---: | :--- | :--- |
| **PA4** | CS | SPI Chip Select | 통신 제어 |
| **PA5** | SCK | SPI Clock | 클럭 신호 |
| **PA6** | MISO | SPI MISO | 데이터 수신 (Cam → MCU) |
| **PA7** | MOSI | SPI MOSI | 데이터 송신 (MCU → Cam) |
| **PB8** | SCL | I2C Clock | 센서 레지스터 설정 |
| **PB9** | SDA | I2C Data | 센서 레지스터 설정 |
| **PA2** | TX | UART TX | PC로 영상 데이터 전송 |
| **PA3** | RX | UART RX | PC 명령 수신 |
| **5V** | VCC | Power | 5V 전원 공급 |
| **GND** | GND | Ground | 공통 접지 |

---

## 📂 소프트웨어 구조 (Software Architecture)
펌웨어(Firmware)와 PC용 뷰어(Viewer)로 구성되어 있습니다.

```bash
📦 Project Root
 ┣ 📂 Core
 ┃ ┣ 📂 Inc
 ┃ ┃ ┗ 📜 ov5642_regs.h   # 레지스터 설정값 (해상도, 화질, PCLK 등 최적화)
 ┃ ┗ 📂 Src
 ┃ ┃ ┣ 📜 main.c          # 메인 루프 (FIFO Burst Read -> UART 전송)
 ┃ ┃ ┗ 📜 ArduCAM.c       # 드라이버 (OV5642 초기화 및 JPEG 엔진 제어)
 ┣ 📜 viewer.py             # PC 뷰어 (Python, MJPEG 디코딩 및 화면 출력)
 ┗ 📜 README.md
```

---

## 🚀 실행 방법 (How to Run)

### Step 1. 펌웨어 업로드 (Firmware Flash)
1.  STM32CubeIDE에서 프로젝트를 엽니다.
2.  프로젝트를 **Build** 합니다.
3.  STM32 보드를 연결하고 **Run**을 눌러 펌웨어를 업로드합니다.

### Step 2. 파이썬 환경 설정 (Prerequisites)
PC 뷰어 실행을 위해 필요한 Python 라이브러리를 설치합니다.
```bash
pip install pyserial opencv-python numpy
```

### Step 3. 뷰어 실행 (Run Viewer)
PC와 보드가 연결된 상태에서 파이썬 스크립트를 실행합니다.
*(주의: 코드 내 `COM_PORT` 변수를 본인의 환경에 맞게 수정해야 합니다.)*
```bash
python viewer.py
```

### Step 4. 시스템 시작 (Start)
터미널에 **"Please press the RESET button"** 메시지가 나타나면, STM32 보드의 **검은색 RESET 버튼**을 한 번 눌러주세요. 잠시 후 영상 스트리밍이 시작됩니다.

---

## ❓ 문제 해결 (Troubleshooting)

| 증상 (Symptom) | 원인 및 해결 방법 (Solution) |
| :--- | :--- |
| **화면이 나오지 않음 (Black Screen)** | 자동 노출(AE)이 초기화 중일 수 있습니다. 카메라를 밝은 곳으로 향하거나 렌즈 캡을 제거해주세요. |
| **가로 줄무늬 노이즈** | 데이터 전송 타이밍 문제입니다. `ov5642_regs.h` 내의 PCLK 설정이 최적화(`0x11` 등) 되었는지 확인하세요. |
| **Terminal: "SPI FAIL"** | SPI 통신 실패입니다. 점퍼 케이블의 연결 상태가 헐겁지 않은지 확인하고 재연결하세요. |
| **영상 끊김 / 딜레이** | UART 속도 문제입니다. 펌웨어와 파이썬 코드 모두 Baud Rate가 **921600**으로 설정되었는지 확인하세요. |

---

## 📝 License
This project is licensed under the MIT License - see the LICENSE file for details.

**Developers**: [Your Name]
