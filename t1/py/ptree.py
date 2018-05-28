import os
import json
import psutil
from time import sleep

def get_subtree(process):
	subtree = {}
	subtree['pid'] = process.pid
	subtree['name'] = process.name()
	# subtree['uid'] = process.uids().real
	# subtree['username'] = process.username()
	children = [get_subtree(child) for child in process.children()]
	subtree['size'] = 1 + sum(child['size'] for child in children)
	subtree['children'] = children

	return subtree

def make_ptree_json(filename='ptree.json'):
	process_tree = get_subtree(psutil.Process(1))
	with open(filename, 'w') as outfile:
		json.dump(process_tree, outfile, indent=4)

def print_ptree(subtree, level=0):
	if level:
		print('|   '*(level-1) + '+' + '\u2014'*3, end='')
	print("{} {} ({})".format(subtree['pid'], subtree['name'], subtree['size']))

	for child in subtree['children']:
		print_ptree(child, level + 1)

def poll_ptree(freq=10):
	while(True):
		process_tree = get_subtree(psutil.Process(1))
		os.system('cls||clear')
		print_ptree(process_tree)
		sleep(freq)
