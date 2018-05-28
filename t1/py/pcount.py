import os
import psutil
from time import sleep
from collections import Counter

# more efficient than Counter?
# user_pcount = {}
# for process in psutil.process_iter():
# 	user = process.uids().real
# 	user_pcount[user] = user_pcount.get(user, 0) + 1

def print_pcount(clear=False):
	count = [p.username() for p in psutil.process_iter()]
	users_pcount = Counter(count)

	if clear:
		os.system('cls||clear')

	print("total process count:", len(count))
	for user in sorted(users_pcount, key=users_pcount.get, reverse=True):
		print(user, end=': ')
		print(users_pcount[user])

def poll_pcount(freq=1):
	while(True):
		print_pcount(True)
		sleep(freq)

print_pcount()