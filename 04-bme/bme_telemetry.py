import time
import serial
import matplotlib.pyplot as plt
from collections import deque

def read_response(ser):
    while True:
        line = ser.readline().decode('ascii').strip()
        if line:
            try:
                return float(line)
            except ValueError:
                continue

def send_command(ser, cmd):
    ser.write(f"{cmd}\n".encode('ascii'))
    time.sleep(0.05)
    return read_response(ser)

def main():
    print("Starting BME280 telemetry...")
    
    try:
        ser = serial.Serial(port='COM3', baudrate=115200, timeout=1.0)
        print(f"Port {ser.name} opened")
    except serial.SerialException as e:
        print(f"Error: {e}")
        return
    
    time.sleep(2)
    ser.reset_input_buffer()
    
    WINDOW_SIZE = 100
    
    measure_temp = deque(maxlen=WINDOW_SIZE)
    measure_pres = deque(maxlen=WINDOW_SIZE)
    measure_hum = deque(maxlen=WINDOW_SIZE)
    measure_ts = deque(maxlen=WINDOW_SIZE)
    
    start_ts = time.time()
    
    plt.ion()
    fig, axes = plt.subplots(3, 1, figsize=(12, 10))
    
    print("\nCollecting data... Press Ctrl+C to stop")
    print("Blow on the sensor to see reaction!\n")
    
    try:
        while True:
            ts = time.time() - start_ts
            
            temp = send_command(ser, "temp")
            pres = send_command(ser, "pres")
            hum = send_command(ser, "hum")
            
            measure_ts.append(ts)
            measure_temp.append(temp)
            measure_pres.append(pres)
            measure_hum.append(hum)
            
            for ax in axes:
                ax.clear()
            
            axes[0].plot(list(measure_ts), list(measure_temp), 'r-', linewidth=2)
            axes[0].set_title('Temperature over time')
            axes[0].set_ylabel('Temperature (°C)')
            axes[0].grid(True, alpha=0.3)
            
            axes[1].plot(list(measure_ts), list(measure_pres), 'b-', linewidth=2)
            axes[1].set_title('Pressure over time')
            axes[1].set_ylabel('Pressure (hPa)')
            axes[1].grid(True, alpha=0.3)
            
            axes[2].plot(list(measure_ts), list(measure_hum), 'g-', linewidth=2)
            axes[2].set_title('Humidity over time')
            axes[2].set_xlabel('Time (s)')
            axes[2].set_ylabel('Humidity (%)')
            axes[2].grid(True, alpha=0.3)
            
            plt.tight_layout()
            plt.pause(0.01)
            
            print(f'T: {temp:.2f}°C, P: {pres:.2f}hPa, H: {hum:.2f}%, t: {ts:.1f}s')
            
            time.sleep(0.5)
            
    except KeyboardInterrupt:
        print("\nStopping...")
        
    finally:
        plt.ioff()
        plt.show()
        ser.close()
        print("Done")

if __name__ == "__main__":
    main()