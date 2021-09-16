#!/usr/bin/python

import struct
from time import sleep

def writeToSeqGPIO(dds_char_dev, address, time, banka, bankb):
  time_bytes = struct.pack('>I', time)
  banka_bytes = struct.pack('>I', banka)
  bankb_bytes = struct.pack('>I', bankb)

  words = []
  #address for time
  address_bytes = struct.pack('>I', address + 0x4000)
  words.append(address_bytes[3:4])
  words.append(address_bytes[2:3])
  words.append(address_bytes[1:2])
  words.append(address_bytes[0:1])
  #time
  words.append(time_bytes[3:4])
  words.append(time_bytes[2:3])
  words.append(time_bytes[1:2])
  words.append(time_bytes[0:1])
  
  #address for bank A
  address_bytes = struct.pack('>I', address + 0x8000)
  words.append(address_bytes[3:4])
  words.append(address_bytes[2:3])
  words.append(address_bytes[1:2])
  words.append(address_bytes[0:1])
  #bank a data
  words.append(banka_bytes[0:1])
  words.append(banka_bytes[1:2])
  words.append(banka_bytes[2:3])
  words.append(banka_bytes[3:4])
  
  #address for bank B
  address_bytes = struct.pack('>I', address + 0xC000)
  words.append(address_bytes[3:4])
  words.append(address_bytes[2:3])
  words.append(address_bytes[1:2])
  words.append(address_bytes[0:1])
  #bank a data
  words.append(bankb_bytes[0:1])
  words.append(bankb_bytes[1:2])
  words.append(bankb_bytes[2:3])
  words.append(bankb_bytes[3:4])

  i = 0

  for word in words:
    print((word).hex(), end=' ')
    i=i+1
    if(i==4):
      print("\n")
      i=0
    if isinstance(word,bytes):
        dds_char_dev.write(word)
    else:
      dds_char_dev.write(word.encode("raw_unicode_escape"))
    dds_char_dev.flush()
    # dds_char_dev.write(word)
    # dds_char_dev.flush()

if __name__ == "__main__":
  print("writing to 4000")
  device = '/dev/axis_fifo_0x0000000080004000';
  dev = open(device, "wb", buffering=0);
  sleep(0.1)
  # with open("/dev/axis_fifo_0x0000000080004000", "wb", buffering=0) as character:
  #note that this sequence leaves all signals high when complete.
  #that makes it easy to verify that all outputs are OK for testing
  #high
  writeToSeqGPIO(dev,0x0000,0x00000000,0x00000001,0x0000ff01)
  sleep(0.05)
  #low after 1 us
  writeToSeqGPIO(dev,0x0001,0x00000064,0x00000000,0x0000ff00)
  sleep(0.05)
  #high another us later
  writeToSeqGPIO(dev,0x0002,0x000000C8,0x00000001,0x00000001)
  

  sleep(0.05)

  writeToSeqGPIO(dev,0x0003,0x000001C8,0xffffffff,0xffffffff)
  sleep(0.05)
  #terminate sequence
  writeToSeqGPIO(dev,0x0004,0x00000000,0x00000000,0x00000000)
  sleep(0.05)
  dev.close()
