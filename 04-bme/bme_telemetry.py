import time
import serial
import matplotlib.pyplot as plt
import numpy as np

def read_value(ser):
    while True:
        try:
            line = ser.readline().decode('ascii').strip()
            value = float(line)
            return value
        except (ValueError, UnicodeDecodeError):
            continue

def main():
    ser = serial.Serial(port='COM3', baudrate=115200, timeout=0.0)
    
    if ser.is_open:
        print(f"Port {ser.name} opened")
    else:
        print(f"Port {ser.name} closed")
    
    measure_temp = []
    measure_pres = []
    measure_hum = []
    measure_ts = []
    
    start_ts = time.time()
    
    ser.write("tm_start\n".encode('ascii'))
    print("Telemetry started")
    
    try:
        while True:
            ts = time.time() - start_ts
            
            ser.write("temp\n".encode('ascii'))
            temp = read_value(ser)
            
            ser.write("pres\n".encode('ascii'))
            pres = read_value(ser)
            
            ser.write("hum\n".encode('ascii'))
            hum = read_value(ser)
            
            measure_ts.append(ts)
            measure_temp.append(temp)
            measure_pres.append(pres)
            measure_hum.append(hum)
            
            print(f'T: {temp:.2f}C, P: {pres:.2f}hPa, H: {hum:.2f}%, t: {ts:.2f}s')
            
            time.sleep(0.5)
            
    except KeyboardInterrupt:
        print("\nStopping...")
        
    finally:
        ser.write("tm_stop\n".encode('ascii'))
        print("Telemetry stopped")
        
        ser.close()
        print("Port closed")
        
        fig, axes = plt.subplots(3, 1, figsize=(12, 10))
        
        axes[0].plot(measure_ts, measure_temp, 'r-', linewidth=1)
        axes[0].set_title('Temperature over time')
        axes[0].set_xlabel('Time (s)')
        axes[0].set_ylabel('Temperature (°C)')
        axes[0].grid(True, alpha=0.3)
        
        axes[1].plot(measure_ts, measure_pres, 'b-', linewidth=1)
        axes[1].set_title('Pressure over time')
        axes[1].set_xlabel('Time (s)')
        axes[1].set_ylabel('Pressure (hPa)')
        axes[1].grid(True, alpha=0.3)
        
        axes[2].plot(measure_ts, measure_hum, 'g-', linewidth=1)
        axes[2].set_title('Humidity over time')
        axes[2].set_xlabel('Time (s)')
        axes[2].set_ylabel('Humidity (%)')
        axes[2].grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.show()

if __name__ == "__main__":
    main()