import os
import subprocess
import serial.tools.list_ports
import sys
from pathlib import Path
import argparse

PROJECT_ROOT = Path(__file__).parent.parent

args = argparse.ArgumentParser()
args.add_argument("-t","--target", type=str, default="esp32pico", help="Target chip")

args.add_argument("-p","--port", type=str, default=None, help="Serial port to flash to")
args.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate for flashing")
args.add_argument("--chip", type=str, default="esp32", help="Chip type")
args.add_argument("--flash_size", type=str, default="detect", help="Flash size")
args.add_argument("--flash_mode", type=str, default="dio", help="Flash mode")
args.add_argument("--flash_freq", type=str, default="40m", help="Flash frequency")
args.add_argument("--bootloader", type=str, default="./bootloader.bin", help="Bootloader file")
args.add_argument("--partitions", type=str, default="./partitions.bin", help="Partitions file")
args.add_argument("--firmware", type=str, default="./firmware.bin", help="Firmware file")
args.add_argument("--spiffs", type=str, default="./spiffs.bin", help="SPIFFS file")

args = args.parse_args()


if args.target == "esp32pico":
    bootloader_address = "0x1000"
    partitions_address = "0x8000"
    firmware_address = "0x20000"
    spiffs_address = "0x320000"
    chip = "esp32"
elif args.target == "esp32c3":
    bootloader_address = "0x0000"
    partitions_address = "0x8000"
    firmware_address = "0x20000"
    spiffs_address = "0x320000"
    chip = "esp32c3"
else:
    print(f"Invalis target: {args.target}")
    sys.exit(0)
    
    
BUILD_DIR = PROJECT_ROOT / f".pio/build/{args.target}"

PLATFROM = sys.platform
if PLATFROM == "linux":
    ESPTOOL = "esptool.py"
elif PLATFROM == "win32":
    ESPTOOL = "esptool"

def select_port():
    # 1. List all available ports
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("No serial ports found!")
        return None

    print("\nAvailable Serial Ports:")
    for i, port in enumerate(ports):
        print(f"[{i}] {port.device} - {port.description}")

    # 2. Get user choice
    try:
        choice = int(input("\nSelect port number: "))
        if 0 <= choice < len(ports):
            return ports[choice].device
    except ValueError:
        pass
    
    print("Invalid selection.")
    return None

def flash_esp32(port):
    # 3. Define your esptool command as a list


    cmd = [
        ESPTOOL, "--port", port, "--chip", chip, "--baud", "115200", 
        "write_flash", "--flash_size", "detect", "--flash_mode", "dio", "--flash_freq", "40m",
        bootloader_address, str(BUILD_DIR/"bootloader.bin"),
        partitions_address, str(BUILD_DIR/"partitions.bin"),
        firmware_address, str(BUILD_DIR/"firmware.bin"),
        spiffs_address, str(BUILD_DIR/"spiffs.bin")
    ]
    mearge_bin_cmd = [
        ESPTOOL, "--chip", chip, "--baud", "115200", 
        "merge_bin", "-o", "firmware.bin", "--flash_mode", "dio", "--flash_size", "4MB",
        bootloader_address, str(BUILD_DIR/"bootloader.bin"),
        partitions_address, str(BUILD_DIR/"partitions.bin"),
        firmware_address, str(BUILD_DIR/"firmware.bin"),
        spiffs_address, str(BUILD_DIR/"spiffs.bin")
    ]
    
    print(f"\nFlashing to {port}...")
    subprocess.run(cmd)
    print(f"\nMearging bins...")
    subprocess.run(mearge_bin_cmd)



if __name__ == "__main__":
    selected_port = select_port()
    if selected_port:
        flash_esp32(selected_port)