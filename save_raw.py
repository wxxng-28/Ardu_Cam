import serial
import time
import re

SERIAL_PORT = 'COM10'
BAUD_RATE = 921600

def main():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
        print(f"Connected to {SERIAL_PORT}. Waiting for Capture...")
    except Exception as e:
        print(f"Error: {e}")
        return

    buffer = bytearray()
    capture_started = False
    expected_len = 0
    
    while True:
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            buffer.extend(data)
            
            # 1. Look for Length Message "Len: 12345"
            if not capture_started:
                try:
                    text_data = buffer.decode('utf-8', errors='ignore')
                    match = re.search(r"Len: (\d+)", text_data)
                    if match:
                        expected_len = int(match.group(1))
                        print(f"Found Length: {expected_len} bytes")
                        # Find end of "Header: 0xXX 0xXX" line if possible, or just start saving
                        # The MCU sends "Len: ...\r\nHeader: ...\r\n" then Data.
                        # We need to find the start of data.
                        # Ideally, look for 'Header: 0xXX 0xXX\r\n'
                        catch_header = re.search(r"Header: 0x[0-9A-F]{2} 0x[0-9A-F]{2}\r\n", text_data)
                        if catch_header:
                            print("Header detected. Saving data...")
                            # Start index is end of header match
                            header_end = text_data.find(catch_header.group(0)) + len(catch_header.group(0))
                            # Convert text position to byte position is risky due to utf-8.
                            # Let's verify by finding the byte pattern of \r\n
                            raw_header_pattern = b'\r\n'
                            # Simplified: Just grab the last expected_len bytes once buffer is big enough
                            capture_started = True
                            buffer = buffer[-expected_len:] # Reset buffer to roughly data start? 
                            # No, safer to just wait until we have enough bytes
                        
                except:
                    pass
            
            # Simple Mode: If we have > 200,000 bytes, JUST SAVE IT.
            if len(buffer) > 200000:
                print(f"Captured {len(buffer)} bytes. Saving to 'raw_image.bin'...")
                with open('raw_image.bin', 'wb') as f:
                    # Try to save only the data tail if possible, but saving all is safer for analysis
                    f.write(buffer)
                print("Done. Please upload 'raw_image.bin'.")
                break
                
        time.sleep(0.01)

if __name__ == '__main__':
    main()
