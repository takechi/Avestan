cd ..

rem command.csv => command.hpp
tools\cmd2hdr.py man\command.csv > src\server\command.hpp

rem command.csv => command.txt
tools\cmd2txt.py man\command.csv > man\command.txt

rem command.csv => command.xml
tools\cmd2xml.py man\command.csv > man\command.xml

rem changelog.csv => changelog.xml
tools\log2xml.py man\changelog.csv > man\changelog.xml

rem changelog.csv => history.xml
tools\log2xml.py man\changelog.csv man\changelog020.csv man\changelog010.csv man\changelog000.csv > man\history.xml

rem generate index.html
msxsl.exe man\index.xml -pi -o man\index.html

rem generate history.html
msxsl.exe man\history.xml man\history.xsl -o man\history.html
