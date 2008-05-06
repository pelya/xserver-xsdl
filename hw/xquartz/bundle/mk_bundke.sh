#!/bin/sh
#
# 'Cause xcodebuild is hard to deal with

BUNDLE_ROOT=$1

mkdir -p ${BUNDLE_ROOT}/Contents/MacOS
[ -d ${BUNDLE_ROOT}/Contents/MacOS ] || exit 1

mkdir -p ${BUNDLE_ROOT}/Contents/Resources/English.lproj/main.nib
[ -d ${BUNDLE_ROOT}/Contents/Resources/English.lproj/main.nib ] || exit 1

if [[ $(id -u) == 0 ]] ; then
	OWNERSHIP="-o root -g admin"
else
	OWNERSHIP=""
fi

localities="Dutch English French German Italian Japanese Spanish da fi ko no pl pt pt_PT ru sv zh_CN zh_TW"
for lang in ${localities} ; do
    for f in InfoPlist.strings Localizable.strings main.nib/keyedobjects.nib ; do
	if [[ $(id -u) == 0 ]] ; then
	        install ${OWNERSHIP} -m 644 Resources/${lang}.lproj/$f ${BUNDLE_ROOT}/Contents/Resources/${lang}.lproj/${f}
	else
	        install ${OWNERSHIP} -m 644 Resources/${lang}.lproj/$f ${BUNDLE_ROOT}/Contents/Resources/${lang}.lproj/${f}
	fi
    done
done

install ${OWNERSHIP} -m 644 Resources/English.lproj/main.nib//designable.nib ${BUNDLE_ROOT}/Contents/Resources/English.lproj/main.nib
install ${OWNERSHIP} -m 644 Resources/X11.icns ${BUNDLE_ROOT}/Contents/Resources

install ${OWNERSHIP} -m 644 Info.plist ${BUNDLE_ROOT}/Contents
install ${OWNERSHIP} -m 644 PkgInfo ${BUNDLE_ROOT}/Contents
