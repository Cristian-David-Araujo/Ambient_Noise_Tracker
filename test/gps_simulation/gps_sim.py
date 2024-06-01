import serial
import time
from datetime import datetime

def get_nmea_sentence(latitude, longitude):
    """
    Genera una oración NMEA GPGGA con la latitud y longitud proporcionadas.
    """
    lat_deg = int(abs(latitude))
    lat_min = (abs(latitude) - lat_deg) * 60
    lat_hemi = 'N' if latitude >= 0 else 'S'

    lon_deg = int(abs(longitude))
    lon_min = (abs(longitude) - lon_deg) * 60
    lon_hemi = 'E' if longitude >= 0 else 'W'

    utc_time = datetime.utcnow().strftime('%H%M%S.00')
    nmea_sentence = f'GPGGA,{utc_time},{lat_deg:02d}{lat_min:07.4f},{lat_hemi},{lon_deg:03d}{lon_min:07.4f},{lon_hemi},1,08,0.9,545.4,M,46.9,M,,'
    checksum = calculate_checksum(nmea_sentence)
    return f'${nmea_sentence}*{checksum:02X}'

def calculate_checksum(sentence):
    """
    Calcula el checksum para una oración NMEA.
    """
    checksum = 0
    for char in sentence:
        checksum ^= ord(char)
    return checksum

def main():
    # Configurar la conexión serial (ajusta '/dev/serial0' si es necesario)
    serial_port = serial.Serial(
        port='/dev/serial0', 
        baudrate=9600, 
        timeout=1
    )

    latitude = 37.7749  # Reemplaza con la latitud deseada
    longitude = -122.4194  # Reemplaza con la longitud deseada

    try:
        while True:
            nmea_sentence = get_nmea_sentence(latitude, longitude)
            serial_port.write((nmea_sentence + '\r\n').encode('ascii'))
            time.sleep(1)
    except KeyboardInterrupt:
        print("Detenido por el usuario")
    finally:
        serial_port.close()

if __name__ == "__main__":
    main()
