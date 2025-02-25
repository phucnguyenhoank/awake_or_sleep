import os

input_file = "sleep_model.tflite"
output_file = "model.h"

with open(input_file, "rb") as f:
    data = f.read()

array_name = "sleep_model_tflite"
with open(output_file, "w") as f:
    f.write(f"unsigned char {array_name}[] = {{\n")
    for i, byte in enumerate(data):
        f.write(f"0x{byte:02x}")
        if i < len(data) - 1:
            f.write(", ")
        if (i + 1) % 12 == 0:  # Line break every 12 bytes for readability
            f.write("\n")
    f.write("\n};\n")
    f.write(f"unsigned int {array_name}_len = {len(data)};\n")
print(f"Generated {output_file} with {len(data)} bytes.")