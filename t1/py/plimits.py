import os
import psutil

def check_root():
	p = psutil.Process()
	error_str = "You must run this program as root to change resource limits"
	if os.getuid() != 0:
		raise RuntimeError(error_str)

def get_user_process(username=None, uid=None):
	if username==None and uid==None:
		raise ValueError("Specify user by name or id")

	for process in psutil.process_iter():
		if process.username() == username or process.uids().real == uid:
			return process

	raise RuntimeError("User not found")


def get_limits(username=None, uid=None):
	p = get_user_process(username, uid)
	return p.rlimit(psutil.RLIMIT_NPROC)


def set_limits(limits, username=None, uid=None):
	check_root()
	p = get_user_process(username, uid)
	p.rlimit(psutil.RLIMIT_NPROC, limits)