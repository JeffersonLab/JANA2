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
	if ($release_string =~ /^Fedora release/) {
	    @token = split(/\s+/, $release_string);
	    $release = "_Fedora$token[2]";
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
	} elsif ($release_string =~ /^Red Hat Enterprise Linux Workstation release 6.*/) {
	    $release = '_RHEL6';
	} elsif ($release_string =~ /^CentOS release 5.*/) {
	    $release = '_CentOS5';
	} elsif ($release_string =~ /^CentOS release 6.*/) {
	    $release = '_CentOS6';
	} elsif ($release_string =~ /^Scientific Linux SL release 5.*/ ) {
	    $release = '_SL5';
	  }
	else {
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
 	} elsif ($release_string =~ /^12.*/) {
	    $release = '_macosx10.8';
 	} elsif ($release_string =~ /^13.*/) {
	    $release = '_macosx10.9';
	} else {
	    print STDERR "unrecognized Mac OS X (Darwin) release\n";
	    $release = '_macosx';
	}
} else {
    $release = '';
}


# Set the default compiler version number (may be overridden below)
$ccversion = `cc -dumpversion`;
chomp $ccversion;

# Decide if we are using gcc or clang or an "other" compiler type
$compiler_type = "cc";
$compiler_version_str = `cc -v 2>&1`;
if ($compiler_version_str =~ /\sgcc version\s/) {

	$compiler_type = "gcc";

} elsif ($compiler_version_str =~ /clang version\s+/) {

	# clang seems to report different numbers for the version
	# if you use "clang -dumpversion" or "clang -v". The former
	# seems to correspond to the installed gcc version number
	# while the later the actual clang version number. Extract
	# the clang version number here, replacing the one obtained
	# via "cc -dumpversion" from above.
	$compiler_type = "clang";
	$' =~ /\s/;
	$ccversion = $`;

} elsif ($compiler_version_str =~ /Apple LLVM version\s+/) {

	# Starting with OS X 10.9 (Mavericks) the "cc -v" command
	# prints things like this:
	#    Apple LLVM version 5.0 (clang-500.2.79) (based on LLVM 3.3svn)
	#    Target: x86_64-apple-darwin13.0.0
	#    Thread model: posix
	#
	# (The "-dumpversion" option still seems to report the gcc version
	# as decribed above).
	#
	# It seems they've switched from reporting it as the "clang version"
	# to the "LLVM version". We follow their lead here by making the compiler
	# type "llvm".
	$compiler_type = "llvm";
	$' =~ /\s/;
	$ccversion = $`;
}


# Set the processor type
# We fall back to the type reported by uname -p, but only if we
# can't get the type from the cc -v result. The reason is that on
# Mac OS X, uname -p will report "i386" even though the system
# is x86_64 and the compiler builds 64-bit executables.
$processor = `uname -p`;
chomp $processor;
if ( $compiler_version_str =~ /Target: x86_64/ ){
	$processor = "x86_64";
}elsif ( $compiler_version_str =~ /Target: i686-apple-darwin/ ){
	# stubborn Apple still tries to report i686 even for gcc
	# compiler that produces x86_64 executables!!
	$processor = "x86_64";
}

# If the compiler_version variable is not set, use the gcc version
if ($compiler_version eq '') {
	$compiler_version = "${compiler_type}${ccversion}";
}

# If the processor variable is set to "unknown" (Ubuntu systems)
# then use the machine name.
if ($processor eq 'unknown') {
	$processor = `uname -m`;
	chomp $processor;
}

# Finally, form and print the complete string to stdout
print "${uname}${release}-${processor}-${compiler_version}\n";
exit;
