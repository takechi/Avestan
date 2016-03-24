import csv, fileinput

reader = csv.reader(fileinput.input())

for row in reader:
	print '%-30s| %s' % (row[0], row[1])
