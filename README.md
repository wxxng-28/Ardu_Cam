# STM32 ArduCAM OV5642 듀얼 스트림 시스템

**STM32F103RB (Nucleo)**와 **ArduCAM OV5642 (5MP)**를 활용한 **지능형 이미지 캡처/전송 시스템**입니다.  
PC를 통한 **실시간 디버깅**과 FPGA(ZYBO)를 위한 **고속 데이터 전송**을 동시에 처리할 수 있는 **Dual-Stream 아키텍처**를 구현했습니다.

---

## 📖 프로젝트 개요 (Overview)

이 프로젝트는 제한된 MCU 자원 환경에서도 카메라 데이터를 효율적으로 처리합니다.
*   **Debug Channel**: UART2를 통해 PC에서 실시간 영상을 확인하고 원격으로 스냅샷을 저장할 수 있습니다.
*   **Data Channel**: UART1로 트리거 신호를 받으면, 즉시 SPI2를 통해 고속으로 이미지 데이터를 FPGA(ZYBO)로 전송합니다.

---

## ✨ 주요 기능 (Key Features)

### 1. 듀얼 출력 시스템 (Dual Output System)
*   **PC 스트리밍 (UART2)**: `921600` bps 고속 시리얼 통신으로 끊김 없는 MJPEG 비디오 모니터링.
*   **ZYBO 연동 (SPI2)**: FPGA의 요청이 있을 때만 데이터를 전송하는 On-Demand 방식 구현.

### 2. 스마트 트리거 메커니즘 (Trigger Mechanisms)
*   **FPGA 트리거**: ZYBO에서 UART1으로 `'s'` 문자를 보내면, 즉시 현재 프레임을 캡처하여 SPI2로 전송.
*   **매뉴얼 트리거 (버튼)**: 보드의 **파란색 버튼(Blue Button)**을 누르면, PC 스크립트가 이를 감지하고 사진을 파일로 저장.

### 3. 하드웨어 가속 및 화질 제어
*   **Hardware JPEG**: OV5642 내장 JPEG 엔진을 활용해 MCU 부하 최소화.
*   **Burst Read**: FIFO 메모리 버스트 읽기 모드로 데이터 처리 속도 극대화.
*   **Smart Tuning**: 자동 노출(AE), 안티 밴딩(Anti-Banding) 및 타이밍 튜닝 완료.

---

## 🔌 하드웨어 연결 (Hardware Pinout)

### 1. 카메라 연결 (SPI1) - ArduCAM 모듈
| STM32 Pin | ArduCAM Pin | 기능 (Function) |
| :---: | :---: | :--- |
| **PA4** | CS | SPI1 Chip Select |
| **PA5** | SCK | SPI1 Clock |
| **PA6** | MISO | SPI1 MISO (데이터 수신) |
| **PA7** | MOSI | SPI1 MOSI (명령 전송) |
| **PB6/PB7** | SDA/SCL | I2C (레지스터 설정) |

### 2. ZYBO FPGA 연결 (SPI2 + UART1)
| STM32 Pin | ZYBO Pin | 기능 (Function) |
| :---: | :---: | :--- |
| **PB13** | SCK | SPI2 Clock (데이터 전송) |
| **PB15** | MOSI | SPI2 MOSI (데이터 전송) |
| **PB12** | CS | SPI2 CS (사용 시) |
| **PA10** | TX (FPGA) | UART1 RX (트리거 수신) |
| **GND** | GND | 공통 접지 (필수 연결) |

### 3. PC 연결 (UART2) - USB 디버깅
| STM32 Pin | PC Connection | 기능 (Function) |
| :---: | :---: | :--- |
| **PA2** | USB RX | UART2 TX (영상 송신) |
| **PA3** | USB TX | UART2 RX (제어 수신) |

---

## 🛠️ 사용 방법 (How to Use)

### 1. 펌웨어 업로드
1.  STM32CubeIDE에서 프로젝트를 열고 Build 합니다.
2.  Nucleo 보드에 Flash합니다.
3.  **부팅 확인**: 시리얼 터미널(921600 baud)에서 아래 메시지가 뜨면 정상입니다.
    *   `SPI OK`
    *   `Sensor OK`
    *   `Reg Verify OK`

### 2. PC 뷰어 실행 (영상 확인)
카메라 영상을 실시간으로 모니터링합니다.
```bash
python viewer.py
```

### 3. 스냅샷 저장 (매뉴얼 트리거)
평소엔 영상만 보다가, 필요한 순간에만 사진을 저장합니다.
1.  스크립트 실행: `python save_snapshot.py`
2.  보드의 **파란색 버튼(Blue Button)** 클릭.
3.  `>> [TRIGGER] Button Detected!` 메시지와 함께 `./snapshots` 폴더에 사진 저장.

### 4. ZYBO 연동 (FPGA)
FPGA가 이미지를 필요로 할 때만 요청하는 방식입니다.
1.  **ZYBO -> STM32**: UART로 문자 `'s'` (0x73) 전송.
2.  **STM32**: 신호를 받으면 사진 촬영 후 SPI2로 JPEG 데이터 전송.
    *   데이터 포맷: `0xFF 0xD8` (Start) ... `0xFF 0xD9` (End)

---

## ⚠️ 문제 해결 (Troubleshooting)

**Q1. `Sensor FAIL` 또는 `Regs FAIL`이 뜹니다.**
*   카메라 모듈이 꽉 꽂혔는지 확인하세요.
*   I2C 선(SDA, SCL) 연결을 확인하세요.
*   **참고**: 이 프로젝트는 OV5642의 16비트 레지스터 주소를 사용합니다. (8비트 주소 사용 시 ID 읽기 실패함)

**Q2. 영상이 끊기거나 깨집니다.**
*   모든 UART 통신 속도가 **921600**으로 설정되었는지 확인하세요.
*   USB 케이블 품질에 따라 노이즈가 탈 수 있습니다.

**Q3. 보드가 계속 리셋됩니다.**
*   새로 연결한 ZYBO 연결선(SPI2, UART1)에 쇼트(Short)가 없는지 확인하세요.
*   전원 공급이 충분한지 확인하세요.

---

## 📜 라이선스 (License)
이 프로젝트는 교육 및 연구 목적으로 자유롭게 사용할 수 있습니다.
