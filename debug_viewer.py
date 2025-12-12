import serial
import time

SERIAL_PORT = 'COM10'  # Check your COM port
BAUD_RATE = 921600

def main():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"Connected to {SERIAL_PORT}. Listening for ANY data...")
    except Exception as e:
        print(f"Error: {e}")
        return

    while True:
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            # Print data as text (if possible) and hex
            try:
                text = data.decode('utf-8', errors='ignore')
                print(f"Text: {text}")
            except:
                pass
            
            hex_str = ' '.join(f'{b:02X}' for b in data)
            print(f"HEX: {hex_str[:100]} ...") # Print first 100 bytes of chunk
            
            # Check for JPEG Start
            if b'\xff\xd8' in data:
                print("\n!!! FOUND JPEG START (FF D8) !!!\n")
                
            time.sleep(0.1)

if __name__ == '__main__':
    main()
