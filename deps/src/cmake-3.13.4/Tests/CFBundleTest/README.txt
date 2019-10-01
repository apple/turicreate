
CFBundle test project.  The generated .plugin/ bundle from either makefiles or Xcode should look like this:

./Contents
./Contents/Info.plist
./Contents/MacOS
./Contents/MacOS/CFBundleTest
./Contents/Resources
./Contents/Resources/English.lproj
./Contents/Resources/English.lproj/InfoPlist.strings
./Contents/Resources/English.lproj/Localized.rsrc

file Contents/MacOS/CFBundleTest should return something like:
Contents/MacOS/CFBundleTest: Mach-O 64-bit bundle x86_64

It is okay if it is a 32 bit binary; if it is not Mach-O, or is spelled differently, it is not okay.
