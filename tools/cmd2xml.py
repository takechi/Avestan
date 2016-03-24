import csv, fileinput

reader = csv.reader(fileinput.input())

print '<?xml version="1.0" encoding="Shift_JIS"?>'
print '<command-table>'

for row in reader:
	print '\t<command><name>%s</name><description>%s</description></command>' % (row[0], row[1])

print '</command-table>'
