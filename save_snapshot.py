import serial
import time
import os
import struct
from datetime import datetime

# ==========================================
# Configuration
# ==========================================
SERIAL_PORT = 'COM10'  # Change to your port
BAUD_RATE = 921600
SAVE_FOLDER = "./snapshots"

# Ensure save folder exists
if not os.path.exists(SAVE_FOLDER):
    os.makedirs(SAVE_FOLDER)

def main():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.5)
        print(f"Connected to {SERIAL_PORT} at {BAUD_RATE}")
        print("Press 'Enter' to trigger capture, or press the Blue Button on the board.")
    except Exception as e:
        print(f"Error opening serial port: {e}")
        return

    buffer = bytearray()
    
    try:
        while True:
            # 1. User Input (Trigger)
            # This is a bit tricky in a loop, so we'll just listen mostly.
            # To trigger via Python, we'd need a separate thread or non-blocking input.
            # For simplicity, we just listen and let the user use the Button mostly,
            # OR we send a trigger periodically.
            
            # Simple check for keyboard interrupt is handled by 'try-except'
            
            # 2. Read Serial Data
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                buffer.extend(data)
                
                # Look for JPEG Start (0xFF 0xD8) and End (0xFF 0xD9)
                start_idx = buffer.find(b'\xff\xd8')
                end_idx = buffer.find(b'\xff\xd9')
                
                if start_idx != -1 and end_idx != -1 and end_idx > start_idx:
                    # Extract JPEG data
                    jpg_data = buffer[start_idx : end_idx + 2]
                    
                    # Generate Filename
                    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                    filename = os.path.join(SAVE_FOLDER, f"snapshot_{timestamp}.jpg")
                    
                    # Save File
                    with open(filename, "wb") as f:
                        f.write(jpg_data)
                    
                    print(f" >> [SAVED] {filename} ({len(jpg_data)} bytes)")
                    
                    # Clear buffer up to the end of this frame
                    buffer = buffer[end_idx + 2:]
            
            # Prevent buffer explosion
            if len(buffer) > 200000:
                buffer = bytearray()
                print("Buffer cleared (overflow protection)")

            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\nExiting...")
    finally:
        ser.close()

if __name__ == "__main__":
    main()
