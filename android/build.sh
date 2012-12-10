#!/bin/bash

NAME=pepper
SRCS="src/pepper/app/*.java"
KEYSTORE=/home/preda/cheie/and

SDK=/home/preda/sdk
NDK=/home/preda/ndk
TOOLS=$SDK/platform-tools
AAPT=$TOOLS/aapt
DX=$TOOLS/dx
AJAR=$SDK/platforms/android-10/android.jar
PKRES=bin/resource.ap_
OUT=bin/$NAME-unalign.apk
ALIGNOUT=bin/$NAME.apk
set -e

rm -rf libs
$NDK/ndk-build
#NDK_LOG=1 V=1

OLD_SIZE=`wc -c $ALIGNOUT` || OLD_SIZE=0
OLD_SIZE=($OLD_SIZE)

rm -rf bin
mkdir -p bin/classes gen

echo aapt
$AAPT package -f -m -J gen -M AndroidManifest.xml -S res -I $AJAR -F $PKRES
mkdir -p gen/pepper

echo javac
javac -d bin/classes -classpath bin/classes -sourcepath src:gen -bootclasspath $AJAR $SRCS

echo dx
$DX --dex --output=bin/classes.dex bin/classes 

echo apkbuilder
apkbuilder $OUT -u -z $PKRES -f bin/classes.dex -nf libs

echo jarsigner
 jarsigner -sigFile A -keystore $KEYSTORE-0 -storepass 000000 $OUT A
#jarsigner -sigFile A -keystore $KEYSTORE                     $OUT and

echo zipalign
zipalign -f 4 $OUT $ALIGNOUT

NEW_SIZE=`wc -c $ALIGNOUT`
NEW_SIZE=($NEW_SIZE)

echo $(($NEW_SIZE - $OLD_SIZE)) $NEW_SIZE
