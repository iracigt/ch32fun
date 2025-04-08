from machine import Pin, SPI
import time
import micropython

backpressure = Pin(7, Pin.IN)
stat = Pin(6, Pin.OUT)
spi_cs = Pin(5, Pin.OUT)

spi = SPI(0, baudrate=32_000, bits=8, sck=Pin(2), mosi=Pin(3), miso=Pin(4))

i = 0

payload = bytes(4096)

def fill(a, i):
    @micropython.asm_thumb
    def _fill(r0, r1, r2):
        mov(r3, 0x01)
        tst(r3, r1)
        beq(ALIGNED)

        strh(r3, [r0, 0])
        add(r0, 2)
        sub(r1, 1)
        
        label(ALIGNED)
        cmn(r1, r1)
        beq(DONE)
        
        mov(r3, r2)
        mov(r4, 16)
        lsl(r3, r4)
        orr(r3, r2)
        
        label(LOOP)
        str(r3, [r0, 0])
        add(r0, 4)
        sub(r1, 2)
        bgt(LOOP)
        label(DONE)
    _fill(a, len(a), i)

while True:
    fill(payload, i)
    spi_cs.value(False)
    spi.write(payload)
    spi_cs.value(True)
    stat.value(not stat.value())
    print('*', end='')
    time.sleep(0.001)
    # while backpressure and not backpressure.value():
        # pass
    i = (i + 1) & 0xffff