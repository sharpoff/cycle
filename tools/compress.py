#!/usr/bin/env python3

import sys
import os

import signal

def sigint_handler(signum, frame):
    sys.exit()

signal.signal(signal.SIGINT, sigint_handler)

if (len(sys.argv) == 1 or "--help" in sys.argv or "-h" in sys.argv):
    print(f'usage: {sys.argv[0]} input_directory [output_directory]')
    sys.exit()

is_cubemap = False
generate_mipmaps = False

if "--genmipmap" in sys.argv:
    generate_mipmaps = True

directory = os.fsencode(sys.argv[-1])
output_directory = os.fsencode(os.path.join(sys.argv[-1], "compressed/"))

try:
    os.mkdir(output_directory)
except FileExistsError:
    print(f'\"{output_directory}\" already exist')

for file in os.listdir(directory):
    filename = os.fsdecode(file)
    if any([filename.endswith(".png"), filename.endswith(".jpg")]):
        input_path = os.path.join(os.fsdecode(directory), filename)

        ktx_filename = os.path.splitext(filename)[0] + '.ktx'
        output_path = os.path.join(os.fsdecode(output_directory), ktx_filename)
        
        command = f'toktx --target_type RGBA'

        if generate_mipmaps:
            command = command + ' --genmipmap'

        command = command + f' {output_path} {input_path}'

        print(f'{command}')
        os.system(command)