# STM32 ArduCam Project (SPI Slave Mode)

이 프로젝트는 STM32F103 보드를 사용하여 ArduCam(OV5642)에서 영상을 캡처하고, 두 가지 채널로 데이터를 전송하도록 구성되어 있습니다.
1. **PC (UART2)**: 디버깅 및 실시간 영상 확인용
2. **ZYBO (SPI2)**: FPGA로 영상 데이터 전송 (STM32는 Slave 모드로 동작)

---

## 📌 Pin Configuration (핀 설정)

### 1. Camera Interface (ArduCam OV5642)
| Function | Pin Name | Description |
| :--- | :--- | :--- |
| **CS** | **PA4** | Chip Select (Software Controlled) |
| **SCK** | **PA5** | SPI1 Clock |
| **MISO** | **PA6** | SPI1 MISO (Camera -> MCU) |
| **MOSI** | **PA7** | SPI1 MOSI (MCU -> Camera) |
| **SCL** | **PB8** | I2C1 Clock (Sensor Config) |
| **SDA** | **PB9** | I2C1 Data (Sensor Config) |

### 2. ZYBO Interface (SPI2 Slave)
⚠️ **중요**: 이 핀들은 ZYBO 보드와 연결됩니다. **ZYBO가 Master**여야 합니다.
| Function | Pin Name | Description | Note |
| :--- | :--- | :--- | :--- |
| **SCK** | **PB13** | SPI2 Clock (Input) | ZYBO에서 클럭 공급 |
| **MISO** | **PB14** | SPI2 MISO (Output) | STM32 -> ZYBO 데이터 전송 |
| **MOSI** | **PB15** | SPI2 MOSI (Input) | Trigger 신호 수신 |

### 3. PC Debug Interface (UART2)
| Function | Pin Name | Description |
| :--- | :--- | :--- |
| **TX** | **PA2** | STM32 -> PC (ST-Link/USB) |
| **RX** | **PA3** | PC -> STM32 |

---

## 🚀 How to Run (실행 방법)

### 1. Viewer Setup (PC)
파이썬 뷰어를 사용하여 카메라 동작 여부를 확인할 수 있습니다.

**필수 라이브러리 설치:**
```bash
pip install pyserial opencv-python numpy
```

**실행:**
`setting_cam` 폴더 안의 `viewer.py`를 실행합니다.
```bash
python viewer.py
```

**⚠️ 주의사항:**
1. **COM 포트**: `viewer.py` 파일 내의 `SERIAL_PORT` 변수가 현재 연결된 포트(예: `COM10`)와 일치하는지 확인해야 합니다.
2. **Reset**: 뷰어가 실행된 후 "Waiting for boot messages..." 라고 뜨면 보드의 **RESET 버튼**을 한 번 눌러주세요. (부팅 메시지를 인식해야 영상이 나옵니다)

---

## ⚙️ Logic Overview (동작 원리)

1. **상시 캡처**: STM32는 전원이 켜지면 자동으로 영상을 계속 캡처합니다.
2. **PC 전송**: 캡처된 모든 프레임은 UART2를 통해 PC로 전송됩니다 (Viewer 확인용).
3. **SPI2 Trigger (ZYBO)**:
    - STM32는 평소에 SPI(Slave) 수신 대기 상태입니다.
    - ZYBO(Master)가 아무 데이터나 1바이트를 보내면(Trigger), STM32는 즉시 **현재 프레임**을 SPI2로 전송할 준비를 합니다.
    - 그 후 ZYBO가 클럭을 공급해주면 이미지 데이터를 `PB14(MISO)`를 통해 전송합니다.
    - 전송이 끝나면 다시 대기 상태로 돌아갑니다.

---
**작성일**: 2025.12.14
