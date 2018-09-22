#!/bin/sh

echo "" > CHANGELOG.txt
echo "<< Edyuk Changelog >>" >> CHANGELOG.txt
echo "<< `date -R` >>" >> CHANGELOG.txt
echo "" >> CHANGELOG.txt

svn log -r HEAD:403 >> CHANGELOG.txt

