LIBXML2=$1
TARGETCPU=$2
TARGETTYPE=$3

if [ -z "$2" ]; then
	TARGETCPU=SIMPENTIUMgnu
fi

if [ -z "$3" ]; then
	TARGETTYPE=RTP
fi

echo "LIBXML2 Version:     ${LIBXML2}"
echo "LIBXML2 Target CPU:  ${TARGETCPU}"
echo "LIBXML2 Target Type: ${TARGETTYPE}"

rm -fR src
tar xvzf ${LIBXML2}.tar.gz
mv ${LIBXML2} src
cd src

./configure --with-minimum --with-reader --with-writer --with-regexps --with-threads --with-thread-alloc

find . -name '*.in' -exec rm -fR {} +
find . -name '*.am' -exec rm -fR {} +
rm -fR *.m4
rm -fR *.pc
rm -fR *.pl
rm -fR *.py
rm -fR *.spec
rm -fR .deps
rm -fR AUTHORS
rm -fR bakefile
rm -fR ChangeLog
rm -fR config.guess
rm -fR config.log
rm -fR config.status
rm -fR config.stub
rm -fR config.sub
rm -fR configure
rm -fR COPYING
rm -fR Copyright
rm -fR depcomp
rm -fR doc
rm -fR example
rm -fR INSTALL
rm -fR install-sh
rm -fR libxml.3
rm -fR ltmain.sh
rm -fR Makefile
rm -fR Makefile.tests
rm -fR macos
rm -fR mkinstalldirs
rm -fR missing
rm -fR nanoftp.c
rm -fR nanohttp.c
rm -fR NEWS
rm -fR python
rm -fR README
rm -fR README.tests
rm -fR regressions.xml
rm -fR result
rm -fR runsuite.c
rm -fR runtest.c
rm -fR test
rm -fR test*.c
rm -fR TODO*
rm -fR trio*
rm -fR vms
rm -fR win32
rm -fR xml2*
rm -fR xmllint.c
rm -fR xstc

cd ..

make clean all VXCPU=${TARGETCPU} VXTYPE=${TARGETTYPE}

if [ "${TARGETTYPE}" = "RTP" ]; then
	cp libxml2.so ../../lib/.
else
	cp xml2.out ../../bin/.
fi

cp -R src/include/libxml ../../include/.