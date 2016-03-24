import csv, fileinput

reader = csv.reader(fileinput.input())

print '<?xml version="1.0" encoding="Shift_JIS"?>'
print '<log-table>'

lastrow0 = ''

for row in reader:
	if row[0] != lastrow0:
		if lastrow0:
			print '\t</log>'
		lastrow0 = row[0]
		print '\t<log version="%s">' % row[0]
	print '\t\t<entry><type>%s</type><description>%s</description></entry>' % (row[1], row[2])

print '\t</log>'
print '</log-table>'
