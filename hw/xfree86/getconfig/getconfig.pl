#!/usr/bin/perl

# $DHD: xc/programs/Xserver/hw/xfree86/getconfig/getconfig.pl,v 1.13 2003/09/23 05:12:07 dawes Exp $

#
# Copyright 2003 by David H. Dawes.
# Copyright 2003 by X-Oz Technologies.
# All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
# 
# Except as contained in this notice, the name of the copyright holder(s)
# and author(s) shall not be used in advertising or otherwise to promote
# the sale, use or other dealings in this Software without prior written
# authorization from the copyright holder(s) and author(s).
# 
# Author: David Dawes <dawes@XFree86.Org>.
#


#
# This script takes PCI id information, compares it against an ordered list
# of rules, and prints out the configuration information specified by the
# last matching rule.
#
# This script is called by xf86AutoConfig().
#

# Command line processing.

$GetconfigVersion = v1.0;

$debug = 0;

$myname = $0;
$myname =~ s/.*\///;

$signature = "Xorg Foundation getconfig rules file.  Version: ";

while (@ARGV[0] =~ /^-[A-Za-z]$/) {
    $f = shift;
    SWITCH: {
	if ($f eq "-D") {
	    $debug = 1;
	    last SWITCH;
	}
	if ($f eq "-I") {
	    push(@searchPaths, split(/,/, shift));
	    last SWITCH;
	}
	if ($f eq "-V") {
	    printf STDERR "$myname: Version %vd.\n", $GetconfigVersion;
	    exit 0;
	}
	if ($f eq "-X") {
	    $XorgVersionNumeric = shift;
	    if (!defined($XorgVersionNumeric)) {
		print STDERR "$myname: -X requires the Xorg version.\n";
		exit 1;
	    }
	}
	if ($f eq "-b") {
	    $subsys = oct(shift);
	    if (!defined($subsys)) {
		print STDERR "$myname: -b requires the subsys id.\n";
		exit 1;
	    }
	    last SWITCH;
	}
	if ($f eq "-c") {
	    $class = oct(shift);
	    if (!defined($class)) {
		print STDERR "$myname: -c requires the class value.\n";
		exit 1;
	    }
	    last SWITCH;
	}
	if ($f eq "-d") {
	    $device = oct(shift);
	    if (!defined($device)) {
		print STDERR "$myname: -d requires the device id.\n";
		exit 1;
	    }
	    last SWITCH;
	}
	if ($f eq "-r") {
	    $revision = oct(shift);
	    if (!defined($revision)) {
		print STDERR "$myname: -r requires the device revision.\n";
		exit 1;
	    }
	    last SWITCH;
	}
	if ($f eq "-s") {
	    $subsysVendor = oct(shift);
	    if (!defined($subsysVendor)) {
		print STDERR "$myname: -s requires the subsysVendor id.\n";
		exit 1;
	    }
	    last SWITCH;
	}
	if ($f eq "-v") {
	    $vendor = oct(shift);
	    if (!defined($vendor)) {
		print STDERR "$myname: -v requires the vendor id.\n";
		exit 1;
	    }
	    last SWITCH;
	}
    }
}

printf STDERR "$myname: Version %vd.\n", $GetconfigVersion;

if (defined($XorgVersionNumeric)) {
    $XorgVersionMajor = $XorgVersionNumeric / 10000000;
    $XorgVersionMinor = ($XorgVersionNumeric % 10000000) / 100000;
    $XorgVersionPatch = ($XorgVersionNumeric % 100000) / 1000;
    $XorgVersionSnapshot = $XorgVersionNumeric % 1000;
    $XorgVersion = chr($XorgVersionMajor) . chr($XorgVersionMinor) .
		chr($XorgVersionPatch) . chr($XorgVersionSnapshot);
}

if ($debug) {
    printf STDERR "$myname: Xorg Version: %d, %d.%d.%d.%d, %vd.\n",
	$XorgVersionNumeric, $XorgVersionMajor, $XorgVersionMinor,
	$XorgVersionPatch, $XorgVersionSnapshot, $XorgVersion;
} else {
    printf STDERR "$myname: Xorg Version: %vd.\n", $XorgVersion;
}
  

# The rules here are just basic vendor ID to driver mappings.
# Ideally this is all that would be required.  More complicated configuration
# rules will be provided in external files.

# XXX This set of basic rules isn't complete yet.

@rules = (

# Set the weight for the built-in rules.
['$weight = 500'],

# APM
['$vendor == 0x1142',
	'apm'],

# ARK
['$vendor == 0xedd8',
	'apm'],

# ATI
['$vendor == 0x1002',
	'ati'],

# Chips & Technologies
['$vendor == 0x102c',
	'chips'],

# Cirrus
['$vendor == 0x1013',
	'cirrus'],

# Intel
['$vendor == 0x8086',
	'i810'],
['$vendor == 0x8086 && ($chipType == 0x00d1 || $chipType == 0x7800)',
	'i740'],

# Matrox
['$vendor == 0x102b',
	'mga'],

# Neomagic
['$vendor == 0x10c8',
	'neomagic'],

# Number Nine
['$vendor == 0x105d',
	'i128'],

# NVIDIA
['$vendor == 0x10de || $vendor == 0x12d2',
	'nv'],

# S3
['$vendor == 0x5333 && ($device == 0x88d0 ||' .
			'$device == 0x88d1 ||' .
			'$device == 0x88f0 ||' .
			'$device == 0x8811 ||' .
			'$device == 0x8812 ||' .
			'$device == 0x8814 ||' .
			'$device == 0x8901)',
	's3'],

# S3 virge
['$vendor == 0x5333 && ($device == 0x5631 ||' .
			'$device == 0x883d ||' .
			'$device == 0x8a01 ||' .
			'$device == 0x8a10 ||' .
			'$device == 0x8c01 ||' .
			'$device == 0x8c03 ||' .
			'$device == 0x8904 ||' .
			'$device == 0x8a13)',
	's3virge'],

# S3 Savage
['$vendor == 0x5333 && ($device >= 0x8a20 && $device <= 0x8a22 ||' .
			'$device == 0x9102 ||' .
			'$device >= 0x8c10 && $device <= 0x8c13 ||' .
			'$device == 0x8a25 ||' .
			'$device == 0x8a26 ||' .
			'$device >= 0x8d01 && $device <= 0x8d04 ||' .
			'$device >= 0x8c2a && $device <= 0x8c2f ||' .
			'$device == 0x8c22 ||' .
			'$device == 0x8c24 ||' .
			'$device == 0x8c26)',
	'savage'],

# SIS
['$vendor == 0x1039',
	'sis'],

# SMI
['$vendor == 0x126f',
	'siliconmotion'],

# 3Dfx
['$vendor == 0x121a',
	'tdfx'],

# 3Dlabs
['$vendor == 0x3d3d',
	'glint'],

# Trident
['$vendor == 0x1023',
	'trident'],

# Tseng Labs
['$vendor == 0x100c',
	'tseng'],

# VIA
['$vendor == 0x1106',
	'via'],

# VMware
['$vendor == 0x15ad',
	'vmware'],

);

# Reverse the search path list, since the later rules have higher priority
# than earlier ones (weighting being equal).

@searchPaths = reverse(@searchPaths);

if ($debug) {
    $i = 0;
    for $path (@searchPaths) {
	print STDERR "$myname: Search path $i is: \"$path\".\n";
	$i++;
    }
}

print STDERR "$myname: ", $#rules + 1, " built-in rule", plural($#rules + 1),
			".\n";

for $path (@searchPaths) {
    while (<$path/*.cfg>) {
	@tmp = readRulesFile($_);
	if (defined(@tmp[0])) {
	    push @rules, @tmp;
	}
    }
}

if ($debug) {
    $i = 0;
    for $r (@rules) {
	print STDERR "$myname: rule $i is: \'@$r\'.\n";
	$i++
    }
}

$i = 0;
$e = 0;
$weight = 0;
$w = 0;
for $r (@rules) {
    ($cond, $d, @o) = @$r;
    $result = eval $cond;
    if ($@) {
	print STDERR "$myname: Error evaluating rule $i \'$cond\': $@";
	$e++;
    }
    if ($debug) {
	print STDERR "$myname: rule $i \'$cond\' evaluates to \'$result\'.\n";
    }
    if ($result && defined($d) && $weight >= $w) {
	$driver = $d;
	@opts = @o;
	$w = $weight;
    }
    $i++;
}

print STDERR "$myname: Evaluated $i rule", plural($i),
		" with $e error", plural($e), ".\n";

print STDERR "$myname: Weight of result is $w.\n";

if ($debug) {
    if (defined($driver)) {
	print STDERR "$myname: Driver is \'$driver\'.\n";
    } else {
	print STDERR "$myname: No driver.\n";
    }
    if (defined(@opts)) {
	print STDERR "$myname: options are:\n";
	for $opt (@opts) {
	    print STDERR "\t$opt\n";
	}
    } else {
	print STDERR "$myname: No options.\n";
    }
}

print "$driver\n";
for $opt (@opts) {
    print "$opt\n";
}

exit 0;

# Subroutines.

sub readRulesFile {
    my ($file) = @_;
    my $signatureOK = 0;
    my @r, @tmp;
    my $line, $cont, $prevcont, $fileversion;

    undef @tmp;
    undef @r;

    if (open(RF, "<$file")) {
	$prevcont = 0;
	while (<RF>) {
	    chop;
	    $line = $_;
	    next if ($line =~ /^#/);
	    next if ($line =~ /^\s*$/);
	    if (!$signatureOK) {
		if ($line  =~ /^$signature(.*)$/) {
		    $fileversion = $1;
		    $signatureOK = 1;
		    print STDERR
			"$myname: rules file \'$file\' has version $fileversion.\n";
		    next;
		}
	    }
	    if (!$signatureOK) {
		print STDERR "$myname: file \'$file\' has bad signature.\n";
		close(RF);
		last;
	    }
	    $cont = 0;
	    if ($line =~ s/\\\s*$//) {
		$cont = 1;
	    }
	    if (!$prevcont && $line =~ /^\S+/) {
		if (defined(@tmp[0])) {
		    push(@r,[@tmp]);
		}
		undef @tmp;
	    }
	    if ($prevcont) {
		push(@tmp, pop(@tmp) . $line);
	    } else {
		push(@tmp, $line);
	    }
	    $prevcont = $cont;
	}
	if (defined(@tmp[0])) {
	    push(@r,[@tmp]);
	}
	if (!defined(@r[0])) {
	    print STDERR "$myname: no rules in file \'$file\'.\n";
	} else {
	    print STDERR "$myname: ", $#r + 1,
			" rule", plural($#r + 1),
			" added from file \'$file\'.\n";
	}
    } else {
	print STDERR "$myname: cannot open file \'$file\'.\n";
    }

    return @r;
}

sub plural {
    my ($count) = @_;

    if ($count != 1) {
	return "s";
    } else {
	return "";
    }
}

