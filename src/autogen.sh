#!/bin/bash
echo 1
aclocal --force -I m4
echo 2
autoheader
echo 3
touch NEWS README AUTHORS ChangeLog
echo 4
automake --add-missing
echo 5
automake
echo 6
autoconf
echo all end


