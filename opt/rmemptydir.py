import os

# remove empty directries recursively under current directory
def rmemptydir():
	items = os.listdir(".")
	for i in items:
		try:
			os.chdir(i)
		except OSError, e:
			continue
		try:
			rmemptydir()
			os.rmdir(i)
			print "delete", i
		except OSError, e:
			pass
	os.chdir("..")

if __name__ == "__main__":
	import sys, string;
	print "rmemptydir.py: remove empty directories under", os.getcwd(), "? (Y/N):"
	ans = string.lower(sys.stdin.readline())
	if ans == "y\n" or ans == "yes\n":
		print "removing..."
		rmemptydir()
