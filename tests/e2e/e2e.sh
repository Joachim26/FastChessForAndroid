#!/bin/bash 

set -x

# Compile the random_mover
g++ -O3 -std=c++17 tests/mock/engine/random_mover.cpp -o random_mover

# Compile fast-chess
make -j build=debug $1

# EPD Book Test

OUTPUT_FILE=$(mktemp)
./fast-chess -engine cmd=random_mover name=random_move_1 -engine cmd=random_mover name=random_move_2 \
    -each tc=2+0.02s -rounds 5 -repeat -concurrency 2 \
    -openings file=tests/data/openings.epd format=epd order=random -log file=log.txt 2>&1 | tee $OUTPUT_FILE

if grep -q "WARNING: ThreadSanitizer:" $OUTPUT_FILE; then
    echo "Data races detected."
    exit 1
fi


# Check if "Saved results." is in the output, else fail
if ! grep -q "Saved results." $OUTPUT_FILE; then
    echo "Failed to save results."
    exit 1
fi

# PGN Book Test

OUTPUT_FILE_2=$(mktemp)
./fast-chess -engine cmd=random_mover name=random_move_1 -engine cmd=random_mover name=random_move_2 \
    -each tc=2+0.02s -rounds 5 -repeat -concurrency 2 \
    -openings file=tests/data/openings.pgn format=pgn order=random -log file=log.txt 2>&1 | tee $OUTPUT_FILE_2

if grep -q "WARNING: ThreadSanitizer:" $OUTPUT_FILE_2; then
    echo "Data races detected."
    exit 1
fi

# Check if "Saved results." is in the output, else fail
if ! grep -q "Saved results." $OUTPUT_FILE_2; then
    echo "Failed to save results."
    exit 1
fi
