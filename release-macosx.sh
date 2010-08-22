#!/bin/bash
rm -rf release/Mumble\ for\ iOS\ Beta\ Crash\ Reporter.app/
make -j5
mkdir -p release/Mumble\ for\ iOS\ Beta\ Crash\ Reporter.app/Contents/Frameworks/
touch release/Mumble\ for\ iOS\ Beta\ Crash\ Reporter.app/Contents/Frameworks/libssl.0.9.8.dylib
chmod +w release/Mumble\ for\ iOS\ Beta\ Crash\ Reporter.app/Contents/Frameworks/libssl.0.9.8.dylib
touch release/Mumble\ for\ iOS\ Beta\ Crash\ Reporter.app/Contents/Frameworks/libcrypto.0.9.8.dylib
chmod +w release/Mumble\ for\ iOS\ Beta\ Crash\ Reporter.app/Contents/Frameworks/libssl.0.9.8.dylib
macdeployqt release/Mumble\ for\ iOS\ Beta\ Crash\ Reporter.app -dmg
pushd release
codesign --signature-size 6400 -vvv -s 'Mikkel Krautz' Mumble\ for\ iOS\ Beta\ Crash\ Reporter.app/
zip -r MumbleiOSCrashReporter-MacOSX-$(cat ../VERSION).zip Mumble\ for\ iOS\ Beta\ Crash\ Reporter.app/
popd
