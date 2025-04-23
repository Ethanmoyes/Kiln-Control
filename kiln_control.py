import serial
import time
import platform

try:
    import RPi.GPIO as GPIO
    GPIO_AVAILABLE = True
except ImportError:
    GPIO_AVAILABLE = False

def is_raspberry_pi():
    """Check if the system is running on a Raspberry Pi."""
    return GPIO_AVAILABLE and platform.system() == "Linux"

if is_raspberry_pi():
    GPIO.setmode(GPIO.BCM)
    GPIO.setwarnings(False)

class KilnSection:
    def __init__(self, section_id, port_name, temperature_setpoint, kC, kI, kD, gpio_pin=None, enabled=True):
        self.section_id = section_id
        if is_raspberry_pi():
            self.port_name = (f"/dev/ttyACM{port_name[-1]}")
        else:
            self.port_name = port_name
        self.temperature_setpoint = temperature_setpoint
        self.temperature = float('nan')  # Current temperature
        self.power_level = 0  # Current power level
        self.kC = kC
        self.kI = kI
        self.kD = kD
        self.status = True  # Communication status flag
        self.invalid_count = 0  # Track checksum failures
        self.last_received_time = time.time()  # Timestamp of last valid response
        self.enabled = enabled  # Controls whether the kiln is active
        self.gpio_pin = gpio_pin

        if is_raspberry_pi() and self.gpio_pin is not None:
            GPIO.setup(self.gpio_pin, GPIO.OUT)
            GPIO.output(self.gpio_pin, GPIO.HIGH if self.enabled else GPIO.LOW)
    def __repr__(self):
        return (f"KilnSection(id={self.section_id}, port={self.port_name}, "
                f"setpoint={self.temperature_setpoint}, kC={self.kC}, kI={self.kI}, kD={self.kD}, "
                f"enabled={'ON' if self.enabled else 'OFF'}, status={'OK' if self.status else 'FAIL'})")
    
# class KilnSection:
#     def __init__(self, section_id, port_name, temperature_setpoint, kC, kI, kD, enabled=True):
#         self.section_id = section_id
#         self.port_name = port_name
#         self.temperature_setpoint = temperature_setpoint
#         self.temperature = float('nan') # Current temperature
#         self.power_level = 0  # Current power level
#         self.kC = kC
#         self.kI = kI
#         self.kD = kD
#         self.status = True  # Communication status flag
#         self.invalid_count = 0  # Track checksum failures
#         self.last_received_time = time.time()  # Timestamp of last valid response
#         self.enabled = enabled  # Controls whether the kiln is active

    

def calculate_checksum(data):
    """Calculate XOR checksum."""
    checksum = 0
    for byte in data:
        checksum ^= byte
    return checksum

def encode_message(section):
    """Encodes a message for a given kiln section."""
    message = f"{section.temperature_setpoint:.2f}:{section.kC:.2f}:{section.kI:.2f}:{section.kD:.2f}".encode()
    checksum = calculate_checksum(message)
    return message + f":{checksum}\n".encode()

def parse_response(response, section):
    """Parses and validates response from kiln section."""
    parts = response.split(":")
    if len(parts) < 3:  
        print(f"Invalid response format from {section.port_name}: {response}")
        return None, None

    received_checksum = int(parts[-1])
    raw_message = ":".join(parts[:-1]).encode() + b":"
    calculated_checksum = calculate_checksum(raw_message)

    if calculated_checksum == received_checksum:
        try:
            temperature = float(parts[0])
            power_level = float(parts[1])
            section.invalid_count = 0  
            section.last_received_time = time.time()  
            return temperature, power_level
        except ValueError:
            print(f"Error parsing float values from {section.port_name}: {response}")
            return None, None
    else:
        print(f"Checksum mismatch on {section.port_name}! Data ignored.")
        section.invalid_count += 1
        return None, None

def update_kiln_section(section):
    """Communicate with a single kiln section."""
    if not section.enabled:
        print(f"Kiln {section.section_id} is disabled. Skipping.")
        return

    message_with_checksum = encode_message(section)

    try:
        with serial.Serial(section.port_name, 57600, timeout=1) as ser:
            print(f"Sending to {section.port_name}: {message_with_checksum.decode()}")
            ser.write(message_with_checksum)

            response = ser.readline().decode().strip()
            if response:
                temperature, power_level = parse_response(response, section)
                if temperature is not None and power_level is not None:
                    print(f"Valid response from {section.port_name}: Temp={temperature}, Power={power_level}\n")
                    section.temperature = temperature
                    section.power_level = power_level
                    section.status = "Connected"
                else:
                    section.temperature = float('nan')  # Set temperature to NaN if parsing fails
            else:
                print(f"No response from {section.port_name}.\n")
                section.temperature = float('nan')  # Set temperature to NaN if no response is received

    except serial.SerialException as e:
        print(f"Serial error on {section.port_name}: {e}")
        section.status = "Serial Exception"
        section.temperature = float('nan')  # Set temperature to NaN on serial exception

    if section.invalid_count > 5 or (time.time() - section.last_received_time) > 5:
        section.status = "No Response"
        print(f"KilnSection {section.section_id} marked as FAILED due to repeated errors or timeout.\n")
        section.temperature = float('nan')  # Set temperature to NaN on repeated errors or timeout

def update_all_setpoints(kiln_sections, new_setpoint):
    """Update setpoint of all kiln sections."""
    for section in kiln_sections:
        #if section.enabled:
            section.temperature_setpoint = new_setpoint
        #else: section.temperature_setpoint = 0
    print(f"All enabled kiln setpoints updated to {new_setpoint}°F.")

def update_parameters(kiln_sections, section_id, new_kC, new_kI, new_kD):
    """Update kC, kI, kD parameters of a specific kiln section."""
    for section in kiln_sections:
        if section.section_id == section_id:
            section.kC = new_kC
            section.kI = new_kI
            section.kD = new_kD
            print(f"Updated Kiln {section_id} parameters: kC={new_kC}, kI={new_kI}, kD={new_kD}")
            return
    if section_id == 0:
        for section in kiln_sections:
            section.kC = new_kC
            section.kI = new_kI
            section.kD = new_kD
            print(f"Updated all kilns parameters: kC={new_kC}, kI={new_kI}, kD={new_kD}")
            return
    print(f"Error: Kiln section {section_id} not found.")

def check_status(kiln_sections, section_id):
    """Check and return the status of a specific kiln section."""
    for section in kiln_sections:
        if section.section_id == section_id:
            return f"Kiln {section_id} Status: {'OK' if section.status else 'FAILED'}, Enabled: {'ON' if section.enabled else 'OFF'}"
    return f"Error: Kiln section {section_id} not found."

def toggle_kiln(kiln_sections, section_id, enabled):
    """Enable or disable a specific kiln section."""
    for section in kiln_sections:
        if section.section_id == section_id:
            section.enabled = enabled
            state = "ON" if enabled else "OFF"
            print(f"Kiln {section_id} is now {state}.")
         
        if section_id == 0:
            for section in kiln_sections:
                section.enabled = enabled
                state = "ON" if enabled else "OFF"
                print(f"All kilns are now {state}.")
        if is_raspberry_pi() and section.gpio_pin is not None:
            GPIO.output(section.gpio_pin, GPIO.HIGH if enabled else GPIO.LOW)
    print(f"Error: Kiln section {section_id} not found.")
# Setpoint of > 2000 triggers manual mode, 2000+%power(0-100)


def comm_cycle(kiln_sections):
    """Cycle through all kiln sections, evenly spacing updates within 1 second."""

    index = 0
    section_count = len(kiln_sections)
    #interval = 1 / section_count  # 1 second / 4 kilns = 0.25s per kiln

    while index < section_count:
        #start_time = time.time()
        
        section = kiln_sections[index]
        if section.enabled:
            update_kiln_section(section)

        index = (index + 1) #% section_count  # Cycle through kilns

        #elapsed_time = time.time() - start_time
        #sleep_time = max(0, interval - elapsed_time)  # Maintain consistent timing
        #time.sleep(sleep_time)
        time.sleep(0.2)

if __name__ == "__main__":
    comm_cycle()
