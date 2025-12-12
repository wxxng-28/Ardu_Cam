import serial
import cv2
import numpy as np
import time

# Configuration
SERIAL_PORT = 'COM10'  # Change this to your STM32 COM port
BAUD_RATE = 921600

def check_diagnostics(ser):
    print("="*60)
    print(" [Diagnostics Mode] Please press the RESET button on the board!")
    print("="*60)
    print("Waiting for boot messages...")
    
    spi_ok = False
    sensor_ok = False
    reg_ok = False
    
    start_time = time.time()
    
    while True:
        # Non-blocking read needed if we want to handle mixed content later, 
        # but for init we can just readline with a small timeout from main
        if ser.in_waiting:
            try:
                # Read line-by-line for text messages
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if not line:
                    continue
                
                print(f"[BOARD] {line}")
                
                if "SPI OK" in line:
                    spi_ok = True
                    print(" >> ✅ SPI Connection Confirmed!")
                    
                if "Sensor OK" in line:
                    sensor_ok = True
                    print(" >> ✅ Sensor Connection Confirmed!")
                    
                if "Reg Verify OK" in line:
                    reg_ok = True
                    print(" >> ✅ Register Read/Write Confirmed!")
                    
                if "Warming up Sensor" in line:
                    print("\n" + "="*60)
                    print(" [Diagnostic Results]")
                    print(f" - SPI:    {'✅ OK' if spi_ok else '❌ FAIL'}")
                    print(f" - Sensor: {'✅ OK' if sensor_ok else '❌ FAIL'}")
                    print(f" - Regs:   {'✅ OK' if reg_ok else '❌ FAIL'}")
                    print("="*60)
                    print("Starting Video Stream...")
                    break
                    
            except Exception:
                pass
                
        # 30 second timeout for diagnostics
        if time.time() - start_time > 30:
            print("\n[Timeout] Did you press RESET? No boot messages received.")
            break

def main():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
        print(f"Connected to {SERIAL_PORT} at {BAUD_RATE}")
    except Exception as e:
        print(f"Error opening serial port: {e}")
        return

    # Run Diagnostics First
    check_diagnostics(ser)

    buffer = bytearray()

    print("Waiting for image data...")
    
    last_print = time.time()
    frame_count = 0
    
    while True:
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            buffer.extend(data)
            
            # Look for JPEG Start (0xFF 0xD8) and End (0xFF 0xD9)
            start_idx = buffer.find(b'\xff\xd8')
            end_idx = buffer.find(b'\xff\xd9')
            
            if start_idx != -1 and end_idx != -1 and end_idx > start_idx:
                # Extract JPEG data
                jpg_data = buffer[start_idx : end_idx + 2]
                
                # Decode image
                try:
                    img_np = np.frombuffer(jpg_data, dtype=np.uint8)
                    img = cv2.imdecode(img_np, cv2.IMREAD_COLOR)
                    
                    if img is not None:
                        cv2.imshow('ArduCam Stream', img)
                        frame_count += 1
                        if frame_count % 10 == 0:
                            print(f"Frames: {frame_count} (Last: {len(jpg_data)}B)")
                        
                        if cv2.waitKey(1) & 0xFF == ord('q'):
                            break
                    else:
                        print(f"Decode Fail ({len(jpg_data)}B)")
                except Exception as e:
                    print(f"Error: {e}")
                
                # Clear buffer up to the end of this frame
                buffer = buffer[end_idx + 2:]
            
            # Prevent buffer from growing indefinitely if sync is lost
            if len(buffer) > 100000: # Max buffer size constraint
                if start_idx != -1:
                    buffer = buffer[start_idx:] # Keep from start marker
                else:
                    buffer = bytearray() # Clear garbage
                    
    ser.close()

if __name__ == '__main__':
    main()
