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
    
    save_requested = False
    
    try:
        while True:
            # 2. Read Serial Data
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                buffer.extend(data)
                
                # [NEW] Look for Trigger Marker
                marker_idx = buffer.find(b'SAVE_NOW')
                if marker_idx != -1:
                    print(" >> [TRIGGER] Button Detected! Saving next frame...")
                    save_requested = True
                    # Remove marker from buffer to avoid re-triggering
                    # (Keep data after it, as it might be the start of the image)
                    buffer = buffer[:marker_idx] + buffer[marker_idx + 8:]
                
                # Look for JPEG Start (0xFF 0xD8) and End (0xFF 0xD9)
                start_idx = buffer.find(b'\xff\xd8')
                end_idx = buffer.find(b'\xff\xd9')
                
                if start_idx != -1 and end_idx != -1 and end_idx > start_idx:
                    # Extract JPEG data
                    jpg_data = buffer[start_idx : end_idx + 2]
                    
                    # Only Save if Requested
                    if save_requested:
                        # Generate Filename
                        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                        filename = os.path.join(SAVE_FOLDER, f"snapshot_{timestamp}.jpg")
                        
                        # Save File
                        with open(filename, "wb") as f:
                            f.write(jpg_data)
                        
                        print(f" >> [SAVED] {filename} ({len(jpg_data)} bytes)")
                        save_requested = False # Reset Trigger
                    
                    # Clear buffer up to the end of this frame (Always clear processed frames)
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
