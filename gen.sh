#!/usr/bin/env bash

make clean all

#!/bin/bash

# Define the path to the test.asm file
ASM_FILE="test.asm"

# Define the path to the Test_Files directory
TEST_FILES_DIR="Test_Files"

# Define the path to the main executable
MAIN_EXEC="./main"

# Define the path to the original output
ORG_OUTPUT="org_output"

# Define the path to the new output
NEW_OUTPUT="new_output"

# Check if the Test_Files directory exists
if [ ! -d "$TEST_FILES_DIR" ]; then
  echo "Error: Test_Files directory not found."
  exit 1
fi

# Check if the main executable exists
if [ ! -x "$MAIN_EXEC" ]; then
  echo "Error: main executable not found or is not executable."
  exit 1
fi

# Create new output folder if needed
if [ ! -d "$NEW_OUTPUT" ]; then
  mkdir "$NEW_OUTPUT"
else
  rm "$NEW_OUTPUT"/*
fi

# Loop through each file in the Test_Files directory
for file in "$TEST_FILES_DIR"/*; do
  # Check if the file is a regular file
  if [ -f "$file" ]; then
    # Get the filename without the path
    filename=$(basename "$file")

    # Copy the file to test.asm
    cp "$file" "$ASM_FILE"

    # Run the main executable
    echo "Running main with $file..."
    "$MAIN_EXEC" test.asm

    # Check if MCode.mc file was generated
    if [ -f "MCode.mc" ]; then
      file2="$NEW_OUTPUT/${filename%.*}.mc"

      # Rename MCode.mc to match the original filename
      cp "MCode.mc" "$file2"

      # Run diff on the pair of files
      echo "Running diff $ORG_OUTPUT/${filename%.*}.mc" "$file2"
      diff "$ORG_OUTPUT/${filename%.*}.mc" "$file2"
    else
      echo "Error: MCode.mc file not generated."
    fi
  fi
done

make clean

