import serial
import time
from PIL import Image

# Предварительное вычисление цветов (таблица)
COLOR_MAP = {
    (0, 0, 0): "BLACK",
    (255, 255, 255): "WHITE",
}

def rgb_to_color(r, g, b):
    """Быстрое определение цвета с минимумом условий"""
    # Чёрный и белый
    if r == 0 and g == 0 and b == 0:
        return "BLACK"
    if r == 255 and g == 255 and b == 255:
        return "WHITE"
    
    # Основные цвета
    if r > 200 and g < 80 and b < 80:
        return "RED"
    if r < 80 and g > 200 and b < 80:
        return "GREEN"
    if r < 80 and g < 80 and b > 200:
        return "BLUE"
    
    # Вторичные цвета
    if r > 200 and g > 200 and b < 80:
        return "YELLOW"
    if r < 80 and g > 200 and b > 200:
        return "CYAN"
    if r > 200 and g < 80 and b > 200:
        return "MAGENTA"
    
    # Оттенки серого
    brightness = (r + g + b) // 3
    return "WHITE" if brightness > 128 else "BLACK"

def main():
    try:
        # Открываем и ресайзим изображение
        image = Image.open('image.jpg')
        print(f"Original size: {image.size[0]}x{image.size[1]}")
        
        # Уменьшаем размер для ускорения (растягивать на Pico не умеем)
        # Компромисс: 160x120 (в 4 раза меньше пикселей)
        image = image.resize((160, 120), Image.Resampling.LANCZOS)
        width, height = image.size
        print(f"Resized to: {width}x{height} ({(width*height)} pixels)")
        
        image = image.convert('RGB')
        pixels = list(image.getdata())
        
        # Открываем порт
        ser = serial.Serial(port='COM7', baudrate=115200, timeout=2.0)
        print(f"Port {ser.name} opened")
        
        time.sleep(2)
        ser.reset_input_buffer()
        
        # Очищаем экран перед отправкой
        ser.write(b"disp_screen BLACK\n")
        time.sleep(0.1)
        
        # Собираем команды в буфер (уменьшаем количество вызовов write)
        print("Sending image pixels...")
        start_time = time.time()
        
        buffer_size = 0
        max_buffer = 1024  # отправляем пачками по 1KB
        command_buffer = bytearray()
        
        total_pixels = width * height
        last_progress = 0
        
        for y in range(height):
            for x in range(width):
                idx = y * width + x
                r, g, b = pixels[idx]
                
                color = rgb_to_color(r, g, b)
                
                # Формируем команду с бинарными координатами для скорости
                # disp_px X Y COLOR\n
                command = f"disp_px {x} {y} {color}\n"
                command_buffer.extend(command.encode('ascii'))
                buffer_size += len(command)
                
                # Отправляем, когда буфер заполнился
                if buffer_size >= max_buffer:
                    ser.write(command_buffer)
                    command_buffer = bytearray()
                    buffer_size = 0
                
                # Прогресс
                progress = (idx * 100) // total_pixels
                if progress >= last_progress + 10:
                    last_progress = progress
                    print(f"Progress: {progress}%")
        
        # Отправляем остаток буфера
        if command_buffer:
            ser.write(command_buffer)
        
        elapsed = time.time() - start_time
        print(f"Image transfer completed in {elapsed:.1f} seconds!")
        
        # Читаем ответы от Pico, чтобы не забивать буфер
        time.sleep(0.5)
        while ser.in_waiting:
            print(ser.readline().decode().strip())
        
    except Exception as e:
        print(f"Error: {e}")
        
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Port closed")

if __name__ == "__main__":
    main()