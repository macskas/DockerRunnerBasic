#!/bin/bash -e

PACKAGE_NAME=${PACKAGE_NAME:=docker-runner-php}
PACKAGE_VERSION=${PACKAGE_VERSION:=1.0.0-1}
USERNAME=${GITHUB_ACTOR:=macskas}
CHDATE=$(date +%a", "%d" "%b" "%Y" "%H:%M:%S" "%z)
DEBIAN_CODENAME=$(lsb_release -c -s)
DEBIAN_ID=$(lsb_release -i -s|tr '[:upper:]' '[:lower:]')
DIR_DEBIAN=".github/.debian"
MY_RUNNER_NAME="$DEBIAN_ID-$DEBIAN_CODENAME"
ARCH="amd64"
RELEASE_PREFIX="${PACKAGE_NAME}_${PACKAGE_VERSION}_${MY_RUNNER_NAME}_$ARCH"

do_changelog()
{
cat << EOF > debian/changelog
$PACKAGE_NAME ($PACKAGE_VERSION) unstable; urgency=low

  * Just a release

 -- $USERNAME <$USERNAME@email.fake>  $CHDATE
EOF
}

do_compile()
{
    make clean
    cmake .
    make -j2
    strip docker-runner
}

do_prepare_deb()
{
    rm -rf debian
    cp -r $DIR_DEBIAN debian
    mkdir -p debian/$PACKAGE_NAME/usr/share/docker-runner-php
    mkdir -p debian/$PACKAGE_NAME/etc
    cp docker-runner debian/$PACKAGE_NAME/usr/share/docker-runner-php/docker-runner
    chmod +s debian/$PACKAGE_NAME/usr/share/docker-runner-php/docker-runner
    echo php74 > debian/$PACKAGE_NAME/etc/docker-runner.default_php
}

do_make_deb()
{
    dh_shlibdeps
    dh_compress
    dh_link
    fakeroot dh_gencontrol
    dh_installinit
    fakeroot dh_md5sums

    rm -rf Release
    mkdir -p Release
    dh_builddeb --destdir=Release --filename="$RELEASE_PREFIX.deb"
}

do_release_binary()
{
    tar czvf Release/$RELEASE_PREFIX.tar.gz docker-runner
    return 1
}

do_cleanup()
{
    make clean
    rm -rf debian
}

main()
{
    do_compile
    do_prepare_deb
    do_changelog
    do_make_deb
    do_release_binary
    do_cleanup
}


main
