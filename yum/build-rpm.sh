#!/bin/sh

USER_NAME=milter-manager-build
BUILD_SCRIPT=/tmp/build-milter-manager.sh
VERSION=`cat /tmp/milter-manager-version`

yum update -y
yum install -y rpm-build \
    intltool gettext gtk-doc gcc make glib2-devel ruby ruby-devel
yum clean packages

if ! id $USER_NAME >/dev/null 2>&1; then
    useradd -m $USER_NAME
fi

cat <<EOF > $BUILD_SCRIPT
#!/bin/sh

if [ ! -f ~/.rpmmacros ]; then
    cat <<EOM > ~/.rpmmacros
%_topdir \$HOME/rpm
EOM
fi

mkdir -p rpm/SOURCES
mkdir -p rpm/SPECS
mkdir -p rpm/BUILD
mkdir -p rpm/RPMS
mkdir -p rpm/SRPMS

cp /tmp/milter-manager-$VERSION.tar.gz rpm/SOURCES/
cp /tmp/milter-manager.spec rpm/SPECS/

chmod o+rx . rpm rpm/RPMS rpm/SRPMS

rpmbuild -ba rpm/SPECS/milter-manager.spec
EOF

chmod +x $BUILD_SCRIPT
su - $USER_NAME $BUILD_SCRIPT
