from axis_fifo import AXIS_FIFO
from devices import fifo_devices

from axi_gpio import AXI_GPIO
from devices import gpio_devices

# from dac81416 import DAC81416
import struct
import soft_trigger
import reset_all

from getSeqGPIOWords import getSeqGPIOWords
from time import sleep

class DAC81416:
  """Class to control DAC81416 in project KA012.
  Very simple class just used for testing.
  """

  def __init__(self, device=None, noinit=False):
    if device is None:
      self.fifo = AXIS_FIFO()
    else:
      self.fifo = AXIS_FIFO(device)
    if not noinit:
      #SPI config register
      #sets device in active mode
      #activates streaming mode
      self.fifo.write_axis_fifo("\x00\x03\x0A\x8C")
      #GEN config
      #activate internal/external ref
      # self.fifo.write_axis_fifo("\x00\x04\x3F\x00")
      self.fifo.write_axis_fifo("\x00\x04\x7F\x00")
      #BRDCONFIG - disable broadcast mode
      self.fifo.write_axis_fifo("\x00\x05\x00\x00")
      #SYNCCONFIG - activate LDAC
      self.fifo.write_axis_fifo("\x00\x06\xFF\xFF")
      #TOGGCONFIG0 - leave at default
      self.fifo.write_axis_fifo("\x00\x07\x00\x00")
      #TOGGCONFIG1 - leave at default
      self.fifo.write_axis_fifo("\x00\x08\x00\x00")
      #DACRANGE set to +-10V
      self.fifo.write_axis_fifo("\x00\x0A\xAA\xAA")
      #DACRANGE set to +-10V
      self.fifo.write_axis_fifo("\x00\x0B\xAA\xAA")
      #DACRANGE set to +-10V
      self.fifo.write_axis_fifo("\x00\x0C\xAA\xAA")
      #DACRANGE set to +-10V
      self.fifo.write_axis_fifo("\x00\x0D\xAA\xAA")
      #TRIGGER - leave at default
      self.fifo.write_axis_fifo("\x00\x0E\x00\x00")
      #power down control
      self.fifo.write_axis_fifo("\x00\x09\x00\x00")

      for channel in range(16):
      #  self.set_DAC(channel, 0)
       self.set_DAC(channel, 256*128)

  def set_DAC(self, channel, value):
    assert channel>=0 and channel<=15, 'Invalid channel for DAC81416 in set_DAC'
    val = b"\x00" + struct.pack('B',channel+16) + struct.pack('>H', value)
    if self.fifo is not None:
      self.fifo.write_axis_fifo(val)

class GPIO_seq_point:
  def __init__(self, address, time, outputA, outputB):
    self.address = address
    self.time = time
    self.outputA = outputA
    self.outputB = outputB

class DAC_seq_point:
  def __init__(self, address, time, start, incr, chan, clr_incr=0):
    assert (address >= 0),"invalid address!"
    assert (address <= 1023),"invalid address!"
    assert (time >= 0),"invalid time!"
    assert (time <= 65536*65536-1),"invalid time!"
    assert (clr_incr >= 0),"invalid clr_incr!"
    assert (clr_incr <= 1),"invalid clr_incr!"
    assert (chan >= 0),"invalid channel!"
    assert (chan <= 15),"invalid channel!"
    assert (start >= 0),"invalid start!"
    assert (start <= 65535),"invalid start!"
    assert (incr >= 0),"invalid increment!"
    assert (incr <= 65536*65536-1),"invalid increment!"
    self.address = address
    self.time = time
    self.start = start
    self.clr_incr = clr_incr
    self.incr = incr
    self.chan = chan

class DAC_ramp_tester:
  def __init__(self, dac_a, dac_b, device_seq0, device_seq1, main_seq):
    self.dac_a = dac_a
    DAC81416(dac_a) # initialize DAC
    self.dac_b = dac_b
    DAC81416(dac_b) # initialize DAC
    self.gpio2 = AXI_GPIO(gpio_devices['axi_gpio_2'])
    self.fifo_dac_seq0 = AXIS_FIFO(device_seq0)
    self.fifo_dac_seq1 = AXIS_FIFO(device_seq1)
    self.fifo_main_seq = AXIS_FIFO(main_seq)

  def write_point(self, fifo, point):
    #01XXAAAA TTTTTTTT DDDDDDDD DDDDDDDD
    #phase acc shifts by 12 bit => 4096
    #clr_incr    <= gpio_in(52 downto 52);
    #acc_chan    <= gpio_in(51 downto 48);
    #acc_start   <= gpio_in(47 downto 32);
    #acc_incr    <= gpio_in(31 downto  0);

    fifo.write_axis_fifo(b"\x01\x00" + struct.pack('>H', point.address))
    fifo.write_axis_fifo(struct.pack('>I', point.time))
    fifo.write_axis_fifo(struct.pack('>I', point.clr_incr*16*256*256 + point.chan*256*256 + point.start))
    fifo.write_axis_fifo(struct.pack('>I', point.incr))

  def reset_DAC(self):
    DAC81416(self.dac_a) # initialize DAC
    DAC81416(self.dac_b) # initialize DAC

  def mod_enable(self):
    self.gpio2.set_bit(0, channel=1)

  def mod_disable(self):
    self.gpio2.clear_bit(0, channel=1)

  def mod_report(self):
    print(self.gpio2.read_axi_gpio(channel=1))

  def dac_seq_write_points(self, test = 0):
    points0=[]
    points1=[]
    #step 100 LSB per 10 us

    points0.append(DAC_seq_point( address= 0 , time =  100000 ,start = 65207 ,incr =  0 ,chan= 0 ,clr_incr= 1 ))
    points0.append(DAC_seq_point( address= 1 , time =  100001 ,start = 32768 ,incr =  0 ,chan= 1 ,clr_incr= 1 ))
    points0.append(DAC_seq_point( address= 2 , time =  100002 ,start = 49151 ,incr =  0 ,chan= 2 ,clr_incr= 1 ))
    points0.append(DAC_seq_point( address= 3 , time =  100003 ,start = 32768 ,incr =  0 ,chan= 3 ,clr_incr= 1 ))
    points0.append(DAC_seq_point( address= 4 , time =  100004 ,start = 32768 ,incr =  0 ,chan= 4 ,clr_incr= 1 ))
    points0.append(DAC_seq_point( address= 5 , time =  100005 ,start = int(32768-32768/2)  ,incr =  0 ,chan= 5 ,clr_incr= 1 ))
    points0.append(DAC_seq_point( address= 6 , time =  100006 ,start = int(32768-32768/2)  ,incr =  0 ,chan= 6 ,clr_incr= 1 ))
    points0.append(DAC_seq_point( address= 7 , time =  100007 ,start = 32768 ,incr =  0 ,chan= 7 ,clr_incr= 1 ))
    points0.append(DAC_seq_point( address= 8 , time =  100008 ,start = 32768 ,incr =  0 ,chan= 8 ,clr_incr= 1 ))
    points0.append(DAC_seq_point( address= 9 , time =  100009 ,start = 32768 ,incr =  0 ,chan= 9 ,clr_incr= 1 ))
    points0.append(DAC_seq_point( address= 10 , time =  100010 ,start = 32768 ,incr =  0 ,chan= 10 ,clr_incr= 1 ))
    points0.append(DAC_seq_point( address= 11 , time =  100011 ,start = 32768 ,incr =  0 ,chan= 11 ,clr_incr= 1 ))
    points0.append(DAC_seq_point( address= 12 , time =  100012 ,start = 32768 ,incr =  0 ,chan= 12 ,clr_incr= 1 ))
    points0.append(DAC_seq_point( address= 13 , time =  100013 ,start = 32768 ,incr =  0 ,chan= 13 ,clr_incr= 1 ))
    points0.append(DAC_seq_point( address= 14 , time =  100014 ,start = 32768 ,incr =  0 ,chan= 14 ,clr_incr= 1 ))
    points0.append(DAC_seq_point( address= 15 , time =  100015 ,start = 32768 ,incr =  0 ,chan= 15 ,clr_incr= 1 ))
    points0.append(DAC_seq_point( address= 16 , time =  0 ,start = 0 ,incr =  0 ,chan= 0 ,clr_incr= 0 ))


    #step 100 LSB per 10 us
    points1.append(DAC_seq_point( address= 0 , time =  100000 ,start = 32768 ,incr =  0 ,chan= 0 ,clr_incr= 1 ))
    points1.append(DAC_seq_point( address= 1 , time =  100001 ,start = 32768 ,incr =  0 ,chan= 1 ,clr_incr= 1 ))
    points1.append(DAC_seq_point( address= 2 , time =  100002 ,start = 32768 ,incr =  0 ,chan= 2 ,clr_incr= 1 ))
    points1.append(DAC_seq_point( address= 3 , time =  100003 ,start = 32768 ,incr =  0 ,chan= 3 ,clr_incr= 1 ))
    points1.append(DAC_seq_point( address= 4 , time =  100004 ,start = 32768 ,incr =  0 ,chan= 4 ,clr_incr= 1 ))
    points1.append(DAC_seq_point( address= 5 , time =  100005 ,start = int(32768+32768/2) ,incr =  0 ,chan= 5 ,clr_incr= 1 ))
    points1.append(DAC_seq_point( address= 6 , time =  100006 ,start = int(32768+32768/2) ,incr =  0 ,chan= 6 ,clr_incr= 1 ))
    points1.append(DAC_seq_point( address= 7 , time =  100007 ,start = 32768 ,incr =  0 ,chan= 7 ,clr_incr= 1 ))
    points1.append(DAC_seq_point( address= 8 , time =  100008 ,start = 32768 ,incr =  0 ,chan= 8 ,clr_incr= 1 ))
    points1.append(DAC_seq_point( address= 9 , time =  100009 ,start = 32768 ,incr =  0 ,chan= 9 ,clr_incr= 1 ))
    points1.append(DAC_seq_point( address= 10 , time =  100010 ,start = 32768 ,incr =  0 ,chan= 10 ,clr_incr= 1 ))
    points1.append(DAC_seq_point( address= 11 , time =  100011 ,start = 32768 ,incr =  0 ,chan= 11 ,clr_incr= 1 ))
    points1.append(DAC_seq_point( address= 12 , time =  100012 ,start = 32768 ,incr =  0 ,chan= 12 ,clr_incr= 1 ))
    points1.append(DAC_seq_point( address= 13 , time =  100013 ,start = 32768 ,incr =  0 ,chan= 13 ,clr_incr= 1 ))
    points1.append(DAC_seq_point( address= 14 , time =  100014 ,start = 32768 ,incr =  0 ,chan= 14 ,clr_incr= 1 ))
    points1.append(DAC_seq_point( address= 15 , time =  100015 ,start = 32768 ,incr =  0 ,chan= 15 ,clr_incr= 1 ))
    points1.append(DAC_seq_point( address= 16 , time =  0 ,start = 0 ,incr =  0 ,chan= 0 ,clr_incr= 0 ))

    if test==0:
      for point in points0:
        self.write_point(self.fifo_dac_seq0, point)
      for point in points1:
        self.write_point(self.fifo_dac_seq1, point)
    else:
      for point in points0:
        self.write_point(self.fifo_dac_seq1, point)
      for point in points1:
        self.write_point(self.fifo_dac_seq0, point)




  def dio_seq_write_points(self):
    points=[]
    points.append(GPIO_seq_point(address=0,time=1,outputA=0x00000001,outputB=0x00000001))
    points.append(GPIO_seq_point(address=1,time=80000,outputA=0x00000000,outputB=0x00000000))
    points.append(GPIO_seq_point(address=2,time=160000,outputA=0x00000001,outputB=0x00000001))
    points.append(GPIO_seq_point(address=3,time=6400000,outputA=0x00000000,outputB=0x00000000))
    points.append(GPIO_seq_point(address=4,time=0,outputA=0x00000000,outputB=0x00000000))

    for point in points:
      print ("add: ", point.address)
      print ("time: ", point.time)
      print ("outputA: ", point.outputA)
      print ("outputB: ", point.outputB)

    # with open("/dev/axis_fifo_0x0000000080004000", "r+b") as character:
    for point in points:
        # writeToSeqGPIO(character, point)
      seqWords = getSeqGPIOWords(point)
      print ("write SeqGPIO", seqWords)
      for word in  seqWords:
        print (word)
        self.fifo_main_seq.write_axis_fifo(word[0], MSB_first=False)

def program(tester):
  # tester.fifo_dac_seq.write_axis_fifo("\x00\x0A\x01\x01")
  tester.dac_seq_write_points()
  # tester.dio_seq_write_points()

  # ~ print('Next, we need to enable modulation')
  # ~ print('  tester.mod_enable()')
  # ~ print('Now, we can use the software trigger')
  # ~ print('  trigger()')
  # ~ print('All AXI peripherals can be reset, note this does not disable modulation')
  # ~ print('  reset()')
  # ~ print('Finally, don\'t forget to disable modulation again')
  # ~ print('  tester.mod_disable()')

if __name__ == "__main__":

  tester = DAC_ramp_tester(fifo_devices['DAC81416_0'], fifo_devices['DAC81416_1'], fifo_devices['DAC81416_0_seq'], fifo_devices['DAC81416_1_seq'], fifo_devices['GPIO_seq'])
  reset_all.reset()
  # sleep(1)
  tester.dio_seq_write_points()
  tester.mod_enable()
  sleep(1)
  for s in range(200):
    print('iteration: ' + str(s))
    tester.mod_enable()
    tester.dac_seq_write_points(test = int(s%2))
    tester.mod_enable()
    # sleep(1)
    
    # sleep(1)
    soft_trigger.trigger()
    sleep(0.1)
    #reset_all.reset()
    # tester.mod_disable()
    print('****************************************************************************************')
    sleep(1)


