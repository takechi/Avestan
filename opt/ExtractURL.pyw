import fileinput, clipboard, re

expr = re.compile("(http|mms|rtsp)://[0-9A-Za-z_./\\-]+")

found = []

for line in fileinput.input():
	for m in expr.finditer(line):
		found.append(m.group())

#print "\r\n".join(found)
clipboard.copy("\r\n".join(found))
