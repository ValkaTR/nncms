#!/bin/bash

mv -v abuse.sql abuse_bak.sql

#wget http://abuse.wtf-no.com/tor.csv
#wget http://abuse.wtf-no.com/proxy.csv
#wget http://abuse.wtf-no.com/bad-ip.csv

for ip in `cat proxy.csv`; do
	ip=`echo $ip | tr -d "\r\n"`
	echo "INSERT INTO \`bans\` VALUES(null, 0, \"`date +%s`\", \"$ip\", \"255.255.255.255\", \"Proxy\");" >> abuse.sql
done

for ip in `cat tor.csv`; do
	ip=`echo $ip | tr -d "\r\n"`
	echo "INSERT INTO \`bans\` VALUES(null, 0, \"`date +%s`\", \"$ip\", \"255.255.255.255\", \"TOR Exit Nodes\");" >> abuse.sql
done

for ip in `cat bad-ip.csv`; do
	ip=`echo $ip | tr -d "\r\n"`
	echo "INSERT INTO \`bans\` VALUES(null, 0, \"`date +%s`\", \"$ip\", \"255.255.255.255\", \"Abusers\");" >> abuse.sql
done

#sqlite3 nncms.db -init abuse.sql
