#!/bin/bash
export ADDITIONAL_CFLAGS="-DCONFIRM_REQUIRED=0 -DCONTINOUS_INTEGRATION=1"
export LD_LIBRARY_PATH=/usr/local/lib

# 100x normal speed
FREQUENCY=200000000000
TIMEOUT=$((60 * 10))

if [ -f "./tests/$TEST_PATH" ]; then
    # File exists, so copy it
    cp "./tests/$TEST_PATH" ./SPOS/
    echo "File copied successfully."
else
    # File doesn't exist, so throw an error
    echo "Error: File specified in TEST_PATH doesn't exist."
	echo "./tests/$TEST_PATH"
    exit 1
fi
make all || exit 1

expect -c "set timeout $TIMEOUT; log_file out.log; spawn /utils/avrsimv2/installs/avrsimv2-2.3.5/avrsimv2 -m atmega644 -f $FREQUENCY -b /utils/avrsimv2/installs/avrsimv2-2.3.5/boards/board_xml.so -a /utils/avrsimv2/installs/avrsimv2-2.3.5/boards/psp_V2-V5.xml ./SPOS.elf; expect \"TEST PASSED\" { close }" || exit 1

if grep -q "TEST PASSED" out.log; then
    exit 0  # Success
else
    exit 1  # Test failed
fi