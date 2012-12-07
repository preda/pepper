#!/bin/bash

NAME=pepper
SRCS="src/pepper/app/*.java"
KEYSTORE=/home/preda/cheie/and
KEYALIAS=and

SDK=/home/preda/sdk
NDK=/home/preda/ndk
PLATFORM=$SDK/platforms/android-10
TOOLS=$SDK/platform-tools
AAPT=$TOOLS/aapt
DX=$TOOLS/dx
AJAR=$PLATFORM/android.jar
PKRES=bin/resource.ap_
PROGUARD=/home/preda/proguard/lib/proguard.jar
OUT=bin/$NAME-unalign.apk
ALIGNOUT=bin/$NAME.apk
set -e

$NDK/ndk-build
#NDK_LOG=1 V=1

OLD_SIZE=`wc -c $ALIGNOUT` || OLD_SIZE=0
OLD_SIZE=($OLD_SIZE)

rm -rf bin
mkdir -p bin/classes gen

echo aapt
$AAPT package -f -m -J gen -M AndroidManifest.xml -S res -I $AJAR -F $PKRES
mkdir -p gen/pepper
#sed -e's/package i.v/package v/' gen/i/v/R.java > gen/v/R.java
#rm -r gen/i

echo javac
javac -d bin/classes -classpath bin/classes -sourcepath src:gen -bootclasspath $AJAR $SRCS

#echo proguard
#java -jar $PROGUARD @proguard/proguard.cfg -injars bin/classes -outjar bin/obfuscated.jar -libraryjars $AJAR

echo dx
$DX --dex --output=bin/classes.dex bin/classes 

echo apkbuilder
apkbuilder $OUT -u -z $PKRES -f bin/classes.dex -nf libs

echo jarsigner
#jarsigner -keystore $KEYSTORE -storepass 000000 $OUT $KEYALIAS > /dev/null || 
jarsigner -sigFile A -keystore $KEYSTORE $OUT $KEYALIAS
#-sigalg MD5withRSA -digestalg SHA1

echo zipalign
zipalign -f 4 $OUT $ALIGNOUT

NEW_SIZE=`wc -c $ALIGNOUT`
NEW_SIZE=($NEW_SIZE)

echo $(($NEW_SIZE - $OLD_SIZE)) $NEW_SIZE

#keytool -genkey -v -keystore /home/preda/cheie/and-1 -storepass 000000 -alias A -keyalg RSA -validity 10000 -dname "CN=x"
