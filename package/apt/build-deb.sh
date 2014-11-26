#!/bin/sh
# -*- indent-tabs-mode: nil; sh-basic-offset: 4; sh-indentation: 4 -*-

PACKAGE=$(cat /tmp/build-package)
USER_NAME=$(cat /tmp/build-user)
VERSION=$(cat /tmp/build-version)
DEPENDED_PACKAGES=$(cat /tmp/depended-packages)
BUILD_SCRIPT=/tmp/build-deb-in-chroot.sh

run()
{
    "$@"
    if test $? -ne 0; then
        echo "Failed $@"
        exit 1
    fi
}

if [ ! -x /usr/bin/aptitude ]; then
    run apt-get update
    run apt-get install -y aptitude
fi
run aptitude update -V -D
run aptitude safe-upgrade -V -D -y

if test "$(lsb_release --id --short)" = "Ubuntu"; then
    run aptitude install -V -D -y language-pack-ja
else
    if ! dpkg -l | grep 'ii  locales' > /dev/null 2>&1; then
        run aptitude install -V -D -y locales
        run dpkg-reconfigure locales
    fi
fi

run aptitude install -V -D -y devscripts ${DEPENDED_PACKAGES}
run aptitude clean

case $(lsb_release --codename --short) in
    "lucid")
        /usr/bin/update-alternatives --list ruby > /dev/null 2>&1
        if test $? -ne 0; then
            /usr/bin/update-alternatives --install /usr/bin/ruby ruby /usr/bin/ruby1.8 10
            /usr/bin/update-alternatives --install /usr/bin/ruby ruby /usr/bin/ruby1.9.1 20
        fi
        /usr/bin/update-alternatives --set ruby /usr/bin/ruby1.9.1
        ;;
    "precise")
        /usr/bin/update-alternatives --set ruby /usr/bin/ruby1.9.1
        ;;
esac

if ! id $USER_NAME >/dev/null 2>&1; then
    run useradd -m $USER_NAME
fi

cat <<EOF > $BUILD_SCRIPT
#!/bin/sh

rm -rf build
mkdir -p build

cp /tmp/${PACKAGE}-${VERSION}.tar.gz build/${PACKAGE}_${VERSION}.orig.tar.gz
cd build
tar xfz ${PACKAGE}_${VERSION}.orig.tar.gz
cd ${PACKAGE}-${VERSION}/
cp -rp /tmp/${PACKAGE}-debian debian
ruby_version=\$(ruby -rrbconfig -e "print RbConfig::CONFIG['ruby_version']")
if test \${ruby_version} != "1.9.1"; then
    sed -i'' -e "s/1\.9\.1/\${ruby_version}/g" debian/*.dirs debian/*.install
fi
case \$(lsb_release --codename --short) in
    "lucid")
        sed -i"" -e 's/ruby (>= 1:1.9.3)/ruby1.9.1 (>= 1.9.1)/g' \
                 -e 's/ruby-dev (>= 1:1.9.3)/ruby1.9.1-dev (>= 1.9.1)/g' \
                 -e 's/debhelper (>= 9)/debhelper (>= 7)/' \
                 -e '/libev-dev/d' \
                 -e '/ruby-gnome2-dev/d' debian/control
        sed -i"" -e 's/9/7/' debian/compat
        sed -i"" -e 's,usr/lib/\*,usr/lib,' debian/*.install
        ;;
    "precise")
        sed -i"" -e 's/ruby (>= 1:1.9.3)/ruby1.9.1 (>= 1.9.1)/g' \
                 -e 's/ruby-dev (>= 1:1.9.3)/ruby1.9.1-dev (>= 1.9.1)/g' \
                 -e '/ruby-gnome2-dev/d' debian/control
        ;;
esac
debuild -us -uc
EOF

run chmod +x $BUILD_SCRIPT
run su - $USER_NAME $BUILD_SCRIPT
