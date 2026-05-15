import serial
import time
from PIL import Image

def color_to_string(rgb):
    r, g, b = rgb
    if r > 127 and g < 128 and b < 128:
        return "RED"
    elif r < 128 and g > 127 and b < 128:
        return "GREEN"
    elif r < 128 and g < 128 and b > 127:
        return "BLUE"
    elif r > 127 and g > 127 and b < 128:
        return "YELLOW"
    elif r < 128 and g > 127 and b > 127:
        return "CYAN"
    elif r > 127 and g < 128 and b > 127:
        return "MAGENTA"
    elif r > 200 and g > 200 and b > 200:
        return "WHITE"
    else:
        return "BLACK"

def main():
    try:
        image = Image.open('5 Как работать с дисплеем/Задания/pics/get.jpg')
        width, height = image.size
        
        print(f"Image size: {width}x{height}")
        
        image = image.resize((320, 240))
        width, height = image.size
        print(f"Resized to: {width}x{height}")
        
        image = image.convert('RGB')
        
        ser = serial.Serial(port='COM3', baudrate=115200, timeout=1.0)
        
        if ser.is_open:
            print(f"Port {ser.name} opened")
        else:
            print("Failed to open port")
            return
        
        time.sleep(2)
        
        ser.reset_input_buffer()
        
        print("Sending image pixels...")
        
        pixels = list(image.getdata())
        
        for y in range(height):
            for x in range(width):
                idx = y * width + x
                r, g, b = pixels[idx]
                
                if r == 0 and g == 0 and b == 0:
                    color = "BLACK"
                elif r == 255 and g == 255 and b == 255:
                    color = "WHITE"
                elif r > g and r > b:
                    color = "RED"
                elif g > r and g > b:
                    color = "GREEN"
                elif b > r and b > g:
                    color = "BLUE"
                elif r > 200 and g > 200:
                    color = "YELLOW"
                elif g > 200 and b > 200:
                    color = "CYAN"
                elif r > 200 and b > 200:
                    color = "MAGENTA"
                else:
                    brightness = (r + g + b) // 3
                    if brightness > 128:
                        color = "WHITE"
                    else:
                        color = "BLACK"
                
                command = f"disp_px {x} {y} {color}\n"
                ser.write(command.encode('ascii'))
                
                if (y * width + x) % 1000 == 0:
                    print(f"Progress: {(y * width + x) * 100 // (width * height)}%")
                
                time.sleep(0.001)
        
        print("Image transfer completed!")
        
    except Exception as e:
        print(f"Error: {e}")
        
    finally:
        time.sleep(0.1)
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Port closed")

if __name__ == "__main__":
    main()