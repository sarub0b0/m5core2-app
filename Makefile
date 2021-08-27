# ENVIRONMENT :=  m5stack-core2
ENVIRONMENT :=  m5stack-core2-debug

# PIO := C:/Users/kosay/.platformio/penv/Scripts/platformio.exe
PIO := /mnt/c/Users/kosay/.platformio/penv/Scripts/platformio.exe

all:
	$(PIO) run --environment $(ENVIRONMENT)

upload:
	$(PIO) run --target upload --environment $(ENVIRONMENT)

monitor:
	$(PIO) run --target upload --target monitor --environment $(ENVIRONMENT)

clean:
	$(PIO) run --target clean --environment $(ENVIRONMENT)

update:
	$(PIO) update
