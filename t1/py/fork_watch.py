import argparse
from pcount import *
from ptree import *
from plimits import *

prompt_str = ">> "
loop=True
while loop:
	os.system('clear')
	print ("""
1. Monitor number of processes
2. Monitor process tree
3. Export process tree as json
4. Set limits for number of processes
5. Print limits for number of processes
6. Quit
	""")
	answer = input(prompt_str)
	loop = False
	if answer == "1":
		print("\nChoose polling frequency in seconds")
		freq = float(input(prompt_str))
		poll_pcount(freq)
	elif answer == "2":
		print("\nChoose polling frequency in seconds")
		freq = float(input(prompt_str))
		poll_ptree(freq)
	elif answer == "3":
		print("\nChoose output file name (e.g. ptree.json)")
		filename = input(prompt_str)
		make_ptree_json(filename)
	elif answer == "4":
		print("\nChoose for which user you are setting the limits")
		user = input(prompt_str)
		print("\nGive the soft and hard limits (e.g. 400 500)")
		limits = input(prompt_str)
		soft, hard = [int(x) for x in limits.split()]
		limits = (soft, hard)
		try:
			user = int(user)
		except ValueError:
			set_limits(username=user, limits=limits)
		else:
			set_limits(uid=user, limits=limits)
	elif answer == "5":
		print("\nChoose the user whose limits you want printed")
		user = input(prompt_str)
		try:
			user = int(user)
		except ValueError:
			print(get_limits(username=user))
		else:
			print(get_limits(uid=user))
	elif answer != "6":
		print("\nNot a valid choice, please try again") 
		sleep(1)
		loop = True
