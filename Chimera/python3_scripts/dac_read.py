#!/usr/bin/python

if __name__ == "__main__":
	with open("/dev/axis_fifo_0x0000000080002000", "rb") as character:
		print('Reading...')
		reading = character.read(4)
		print(('Read {} bytes: {} {} {} {}'.format(len(reading), hex((reading[0])), hex((reading[1])), hex((reading[2])), hex((reading[3])))))
