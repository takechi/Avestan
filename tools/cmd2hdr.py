import csv, fileinput

reader = csv.reader(fileinput.input())

print "const CommandEntry COMMANDS[] ="
print "{"

for row in reader:
	print '\t{ _T("%s"), _T("%s") },' % (row[0], row[1])

print "};"
