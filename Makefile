# General targets

.PHONY: all
all: format

# Formatting

.PHONY: format
format:
	find . -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' | grep -v bmi270_sensor_api | xargs clang-format -i
