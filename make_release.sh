VERSION=0.6
BASENAME=dashmm-$VERSION
FILENAME=$BASENAME.tar

mkdir $BASENAME
cp AUTHORS $BASENAME/
cp INSTALL $BASENAME/
cp Makefile $BASENAME/
cp README $BASENAME/
cp LICENSE $BASENAME/
cp -r doc/ $BASENAME/
cp -r demo/ $BASENAME/
cp -r include/ $BASENAME/
cp -r src/ $BASENAME/
mkdir $BASENAME/lib


tar cvf $FILENAME $BASENAME/
gzip $FILENAME

rm -r $BASENAME/