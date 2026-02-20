#!/bin/bash

INPUT_DIR="/home/noah/qmk_firmware/.build/"
OUTPUT_DIR="/mnt/c/Users/Noah/Desktop/Files/Programmioong/Hardware Design/hackpad-attemps/macropad-v2/firmware"

qmk compile -kb noahshpad/firmware -km NoahSheppard

if [ ! -d "$OUTPUT_DIR" ]; then
	echo "Output directory does not exist: $OUTPUT_DIR"
	exit 1
fi

cp -r "${INPUT_DIR}"* "$OUTPUT_DIR/"
cp -r "${INPUT_DIR}"* "../wowki/firmware/"
rm -rf "${INPUT_DIR}"*