import sys
import socket
from time import sleep
from threading import Thread
import Adafruit_BBIO.GPIO as GPIO
import Adafruit_BBIO.ADC as ADC

input_values = {}
n_end = True

def get_value(key, var):
	global input_values, n_end
	while (n_end):
		input_values[var] = ADC.read(key)
		sleep(0.05)

def get(key, var):
	global input_values, n_end
	input_values[var] = GPIO.input(key)
	input_values[var+"_pressed"] = False
	while (n_end):
		prev_value = input_values[var];
		input_values[var] = GPIO.input(key)
		if (prev_value and not input_values[var]):
			input_values[var+"_pressed"] = True
		sleep(0.05)

def communication(server_ip):
	global input_values, n_end
	client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	client.connect((server_ip, 12346))
	while (n_end):
		msg = client.recv(64)
		if (msg == b''):
			raise RuntimeError("Error receiving from server")
		if (msg[0] != ord('1')):
			n_end = False
			break

		msg = "{:.5},{:.5},{:.5},{}".format(input_values["p1"],
			input_values["p2"],input_values["light"],int(input_values["button_pressed"]))
		input_values["button_pressed"] = False
		sent = client.send(msg.encode())
		if sent == 0:
			raise RuntimeError("Error sending to server")

	client.close()


ADC.setup()
GPIO.setup("P9_41", GPIO.IN) #button

input_threads = {}
input_threads["player1"] = Thread(target=get_value, args=("P9_33", "p1"))
input_threads["player2"] = Thread(target=get_value, args=("P9_35", "p2"))
input_threads["light"] = Thread(target=get_value, args=("P9_40", "light"))
input_threads["button"] = Thread(target=get, args=("P9_41", "button"))

for _, t in input_threads.items():
	t.start()

server_ip = sys.argv[1]
connect = Thread(target=communication, args=(server_ip,))
connect.start()

for _, t in input_threads.items():
	t.join()
connect.join()
