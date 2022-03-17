from subprocess import Popen, PIPE, STDOUT
import subprocess
import time
import threading
import json
from tkinter import N


def twos_comp(val, bits):
	if (val & (1 << (bits - 1))) != 0: # if sign bit is set e.g., 8bit: 128-255
		val = val - (1 << bits)        # compute negative value
	return val                         # return positive value as is

def process_data(raw_data):
	arr = raw_data.split(',')
	if len(arr) != 5:
		return
	arr = [int(x, 16) for x in arr]
	coords = [twos_comp(x, 32)/8388608 for x in arr[:3]]
	return {
		"axes": coords,
		"buttons": arr[3],
		"switches": arr[4]
	}	


def reciever():
	with Popen('nios2-terminal', shell=True, stdout=PIPE, stdin=PIPE, stderr=PIPE) as p:

		while True:
			p.stdin.write(b"[hellooooooooooooooo]")
			print(p.stdout.readline())

		for line in p.stdout:
			raw_data = line.decode().strip()
			# ignore empty line and starting "nios-terminal: ..." login thing
			if raw_data == '' or raw_data[0] == 'n':
				continue
			# data = process_data(raw_data)
			print(raw_data)
			
			try:
				p.stdin.write(b"hellooooooooooooooo\n")
				# p.communicate(input=str.encode("hellooooooooooooooooooooooooooooooooooooooooooooooooooo\n"), timeout=0.1)
			except:
				pass


if __name__ == "__main__":
	reciever()

