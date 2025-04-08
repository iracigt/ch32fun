#!/usr/bin/env python3
import time
import usb.core
import usb.util

DATA_REQUEST_START  = 0x41
DATA_REQUEST_STOP   = 0x42

dev = usb.core.find(idVendor=0xcafe)

if dev is None:
    raise ValueError('No TinyUSB (CAFE:xxxx) device found')

print(dev)

# get an endpoint instance
cfg = dev.get_active_configuration()

intf = usb.util.find_descriptor(
    cfg,
    # match the first vendor class interface
    custom_match = lambda i: i.bInterfaceClass == 255
)

if intf is None:
    raise ValueError('No vendor interface found for {}:{}'.format(dev.idVendor, dev.idProduct))

bulk_ep = usb.util.find_descriptor(
    intf,
    # match the first IN endpoint
    custom_match = lambda e: \
    (usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_IN)
)

def ctrl_req(d, r):
    d.ctrl_transfer(
        usb.util.CTRL_OUT | usb.util.CTRL_TYPE_CLASS | usb.util.CTRL_RECIPIENT_INTERFACE,
        r,
        0,
        intf.bInterfaceNumber,
        None
    )

# ctrl_req(dev, DATA_REQUEST_START)

Nk = 8<<10
start = time.monotonic_ns()
x = bulk_ep.read(Nk<<10)
# assert(x[0] == x[-1])
stop = time.monotonic_ns()
elapsed_ms = (stop - start) / 1e6
read_mb = len(x)/1024/1024
speed_mbps = read_mb / (elapsed_ms / 1000)
print("read {} MB in {} ms ({:0.02f} MB/s - {:0.0f} Mbps)".format(read_mb, elapsed_ms, speed_mbps, speed_mbps*8))

