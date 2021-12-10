#!/usr/bin/env bash
cd ..
rm -rf pa1
mkdir pa1
cp -rv lab1 pa1
cd pa1
mv lab1 pa1
tar cfvz pa1.tar.gz pa1
