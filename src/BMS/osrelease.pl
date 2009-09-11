#!/usr/bin/env perl

# Generate a string specific to the current platform for
# purposes of separating and distributing binaries.
#
# The output will have the form:
#
#  OS_flavor##-processor-gcc##
#
# Some examples:
#
# Linux_RHEL5-i686-gcc4.1.2
# Linux_RHEL4-i686-gcc3.4.3
# Linux_FC6-i686-gcc4.1.1
# Linux_Fedora9-i686-gcc4.3.0
# Darwin_macosx10.4-i386-gcc4.0.1
# Linux_Ubuntu6.06-unknown-gcc4.0.3
#
# Errors are printed to stderr and some
# attempt is made to return a reasonable
# string for the system such that the script
# will never fail.
#

# This first section sets the uname and release variables
# which hold the "OS" and "_flavor##" parts of the string.
$uname = `uname`;
chomp $uname;
if ($uname eq 'Linux') {
    if (-e '/etc/fedora-release') {
	$release_string = `cat /etc/fedora-release`;
	if ($release_string =~ /^Fedora release 11.*/) {
	    $release = '_Fedora11';
	} elsif ($release_string =~ /^Fedora release 10.*/) {
	    $release = '_Fedora10';
	} elsif ($release_string =~ /^Fedora release 9.*/) {
	    $release = '_Fedora9';
	} elsif ($release_string =~ /^Fedora release 8.*/) {
	    $release = '_Fedora8';
	} elsif ($release_string =~ /^Fedora release 7.*/) {
	    $release = '_Fedora7';
	} elsif ($release_string =~ /^Fedora Core release 6.*/) {
	    $release = '_FC6';
	} else {
	    print STDERR "unrecognized Fedora release\n";
	    $release = '_Fedora';
	}
    } elsif (-e '/etc/redhat-release') {
	$release_string = `cat /etc/redhat-release`;
	if ($release_string =~ /^Red Hat Enterprise Linux WS release 3.*/) {
	    $release = '_RHEL3';
	} elsif ($release_string =~ /^Red Hat Enterprise Linux WS release 4.*/) {
	    $release = '_RHEL4';
	} elsif ($release_string =~ /^Red Hat Enterprise Linux Client release 5.*/) {
	    $release = '_RHEL5';
	} elsif ($release_string =~ /^CentOS release 4.*/ ){
	    $release = '_CentOS4';
	} elsif ($release_string =~ /^CentOS release 5.*/ ){
	    $release = '_CentOS5';
	} else {
	    print STDERR "unrecognized Red Hat release\n";
	    $release = '_RH';
	}
    } elsif (-e '/etc/lsb-release') { # Ubuntu
    	$distrib_id = `cat /etc/lsb-release | grep DISTRIB_ID`;
	$distrib_id =~ s/DISTRIB_ID=//;
    	$distrib_release = `cat /etc/lsb-release | grep DISTRIB_RELEASE`;
	$distrib_release =~ s/DISTRIB_RELEASE=//;
	chomp $distrib_id;
	chomp $distrib_release;
	$release = "_${distrib_id}${distrib_release}";	
    } else {
	$release = '';
    }
} elsif ($uname eq 'SunOS') {
	$release = '_' . `uname -r`;
	chomp $release;
	@toks = split(/\s/, `CC -V 2>&1`);
	$CC_version =  $toks[3];
	$compiler_version = "CC${CC_version}";
} elsif ($uname eq 'Darwin') {
 	$release_string = `uname -r`;
	if ($release_string =~ /^6.*/) {
	    $release = '_macosx10.2';
 	} elsif ($release_string =~ /^7.*/) {
	    $release = '_macosx10.3';
 	} elsif ($release_string =~ /^8.*/) {
	    $release = '_macosx10.4';
 	} elsif ($release_string =~ /^9.*/) {
	    $release = '_macosx10.5';
 	} elsif ($release_string =~ /^10.*/) {
	    $release = '_macosx10.6';
 	} elsif ($release_string =~ /^11.*/) {
	    $release = '_macosx10.7';
	} else {
	    print STDERR "unrecognized Mac OS X (Darwin) release\n";
	    $release = '_macosx';
	}
} else {
    $release = '';
}

# This part sets the processor type and GCC version number
$processor = `uname -p`;
$gccversion = `gcc -dumpversion`;
chomp $processor;
chomp $gccversion;

# If the compiler_version variable is not set, use the gcc version
if ($compiler_version eq '') {
	$compiler_version = "gcc${gccversion}";
}

# Finally, form and print the complete string to stdout
print "${uname}${release}-${processor}-${compiler_version}\n";
exit;
