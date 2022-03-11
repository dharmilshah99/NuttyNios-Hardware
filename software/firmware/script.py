from subprocess import Popen, PIPE
import subprocess
import time
import threading
import json
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

import paho.mqtt.client as paho


#define callback
def on_message(client, userdata, message):
    time.sleep(1)
    print("received message =",str(message.payload.decode("utf-8")))

client= paho.Client("fpgaclient-001") 
# parameters for authetication
client.username_pw_set(username="nuttynios",password="dharmil")
client.tls_set('./certs/server-certs/ca.crt','./certs/client-certs/client.crt','./certs/client-certs/client.key',tls_version=2)

client.on_message=on_message

client.connect('44.203.98.126', 31883)
client.loop_start() #start loop to process received messages

DATA_ENDPOINT = "node/data"

print(f"publishing to {DATA_ENDPOINT}")

client.disconnect() #disconnect
client.loop_stop() #stop loop


def twos_comp(val, bits):
	if (val & (1 << (bits - 1))) != 0: # if sign bit is set e.g., 8bit: 128-255
		val = val - (1 << bits)        # compute negative value
	return val                         # return positive value as is

def process_data(raw_data):
	arr = raw_data.split(',')
	if len(arr) != 5:
		return
	arr = [int(x, 16) for x in arr]
	coords = [twos_comp(x, 32) for x in arr[:3]]
	coordx = coords[0]
	return {
		"axes": coords,
		"buttons": arr[3],
		"switches": arr[4]
	}

def reciever(endpoint):
	with Popen('C:/intelFPGA_lite/18.0/quartus/bin64/nios2-terminal.exe', shell=True, stdout=PIPE) as p:
		for line in p.stdout:
			raw_data = line.decode().strip()
			# ignore empty line and starting "nios-terminal: ..." login thing
			if raw_data == '' or raw_data[0] == 'n':
				continue
			data = process_data(raw_data)
			client.publish(endpoint, json.dumps(data))#publish
			
			# print(data)

# def run_command(command):
# 	subprocess.run(f'C:/intelFPGA_lite/18.0/quartus/bin64/nios2-terminal.exe <<< {command}',
# 			shell=True)

# def control_thread():
# 	while True:
# 		run_command("1")
# 		time.sleep(1)
# 		print("Number of lines recieved: ", num)
			
def plotter():
	global coordx

	fig, ax = plt.subplots()
	xdata, ydata = [], []
	ln, = plt.plot([], [], 'ro')

	def init():
		ax.set_xlim(0, 2*np.pi)
		ax.set_ylim(-1, 1)
		return ln,

	def update(frame):
		xdata.append(frame)
		ydata.append(coordx)
		ln.set_data(xdata, ydata)
		return ln,

	ani = FuncAnimation(fig, update, frames=np.linspace(0, 2*np.pi, 128),
                    init_func=init, blit=True)
	plt.show()

if __name__ == "__main__":
	plotter = threading.Thread(target=plotter)
	reciever = threading.Thread(target=reciever)

	plotter.start()
	reciever.start()