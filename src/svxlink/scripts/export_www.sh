#!/bin/sh

#GROUPDIR="${HOME}/svxlink"
GROUPDIR="/home/project-web/svxlink"

SVNROOT=svn://svn.code.sf.net/p/svxlink/svn/branches/releases/13.07

if [ ! -d "${GROUPDIR}" ]; then
  echo "*** ERROR: The group directory does not exist. Create it first!"
  exit 1
fi

echo "--- Removing old web directory..."
cd ${GROUPDIR}
rm -rf htdocs
echo

echo "--- Checking out new web directory..."
mkdir -p htdocs/doc
svn export $SVNROOT/www/index.html htdocs/index.html
echo

if [ -d src ]; then
  echo "--- Updating source directory..."
  cd src
  svn up
else
  echo "--- Checking out source directory..."
  svn co $SVNROOT/src
  cd src
fi
echo

echo "--- Building include directory..."
#./create_config.sh .config
touch .config
make makefiles
cd async/core
make expinc
cd ../audio
make expinc
cd ../cpp
make expinc
cd ../qt
make expinc
cd ../../echolib
make expinc
cd ..
echo

echo "--- Building doxygen documentation..."
doxygen doxygen.async
doxygen doxygen.echolib
echo

echo "--- Copying doxygen documentation into web directory..."
cd doc
cp -a async echolib ${GROUPDIR}/htdocs/doc/
echo

echo "--- Generating AsciiDoc documentation..."
asciidoc -n -o ${GROUPDIR}/htdocs/doc/echolink_proxy_protocol.html \
	echolink_proxy_protocol.txt
echo

echo "--- Converting manual pages into HTML..."
cd man
for file in *.{1,5}; do
  echo "  $file"
  section=${file##*.}
  destdir="${GROUPDIR}/htdocs/man/man$section/"
  [ -d $destdir ] || mkdir -p $destdir
  man2html -r $file > $destdir/$file.html
done

