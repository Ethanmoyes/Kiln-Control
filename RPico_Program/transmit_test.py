import serial
import time

# Serial port configuration
SERIAL_PORT = "COM14"
BAUD_RATE = 57600

# Example data
temperature_setpoint = 200  # Example setpoint (0-2000°F)
a, b, c = 27, .68, 0  # Example floating-point parameters
pico_status = True  # Status flag for communication health
invalid_count = 0  # Track consecutive checksum failures
last_received_time = time.time()  # Timestamp of last valid response

# Function to calculate XOR checksum
def calculate_checksum(data):
    checksum = 0
    for byte in data:
        checksum ^= byte
    return checksum

def encode_message(values):
    """Encodes a list of values into a formatted string with checksum."""
    message = ":".join(f"{v:.2f}" for v in values).encode()
    checksum = calculate_checksum(message)
    return message + f":{checksum}\n".encode()

def parse_response(response):
    """Parses the response from the Pico and verifies checksum."""
    global invalid_count, last_received_time
    
    parts = response.split(":")
    if len(parts) < 3:  # Expect at least two data points and a checksum
        print("Invalid response format:", response)
        return None, None  # Return invalid result
    
    received_checksum = int(parts[-1])  # Extract checksum
    raw_message = ":".join(parts[:-1]).encode() + b":"  # Reconstruct for checksum verification
    calculated_checksum = calculate_checksum(raw_message)
    
    if calculated_checksum == received_checksum:
        try:
            temperature = float(parts[0])
            power_level = float(parts[1])
            invalid_count = 0  # Reset failure count
            last_received_time = time.time()  # Update timestamp
            return temperature, power_level
        except ValueError:
            print("Error parsing float values:", response)
            return None, None
    else:
        print(f"Checksum mismatch! Received {received_checksum}, Expected {calculated_checksum}. Data ignored.\n")
        invalid_count += 1
        return None, None

while True:
    # Create and send message
    message_with_checksum = encode_message([temperature_setpoint, a, b, c])

    with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2) as ser:
        print("Sending:", message_with_checksum.decode())
        ser.write(message_with_checksum)  # Send message

        # Wait for response from Pico
        response = ser.readline().decode().strip()

        if response:
            temperature, power_level = parse_response(response)
            if temperature is not None and power_level is not None:
                print(f"Valid response received: Temperature = {temperature}, Power Level = {power_level}\n")
        else:
            print("No response received.\n")

        # Check for communication failure
        if invalid_count > 5 or (time.time() - last_received_time) > 5:
            pico_status = False
            print("Pico status set to FALSE due to repeated checksum failures or timeout.\n")

        time.sleep(1)  # Maintain loop at 1 Hz
