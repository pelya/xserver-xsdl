#!/bin/sh
#
# 'Cause xcodebuild is hard to deal with

BUNDLE_ROOT=$1

localities="Dutch English French German Italian Japanese Spanish da fi ko no pl pt pt_PT ru sv zh_CN zh_TW"
for lang in ${localities} ; do
    mkdir -p ${BUNDLE_ROOT}/Contents/Resources/${lang}.lproj/main.nib
    [ -d ${BUNDLE_ROOT}/Contents/Resources/${lang}.lproj/main.nib ] || exit 1

    for f in InfoPlist.strings Localizable.strings main.nib/keyedobjects.nib ; do
        install -m 644 Resources/${lang}.lproj/$f ${BUNDLE_ROOT}/Contents/Resources/${lang}.lproj/${f}
    done
done

install -m 644 Resources/English.lproj/main.nib//designable.nib ${BUNDLE_ROOT}/Contents/Resources/English.lproj/main.nib
install -m 644 Resources/X11.icns ${BUNDLE_ROOT}/Contents/Resources

install -m 644 Info.plist ${BUNDLE_ROOT}/Contents
install -m 644 PkgInfo ${BUNDLE_ROOT}/Contents

if [[ $(id -u) == 0 ]] ; then
	chown -R root:admin ${BUNDLE_ROOT}
fi
