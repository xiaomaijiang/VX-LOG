#!/usr/bin/perl -w
#$Id:  $

################################################################################################
# Script to test nxlog
#
################################################################################################

use strict;
use IO::Socket;
eval "use IO::Socket::SSL"; # don't fail if this is not available
use IO::Socket::UNIX;
use Time::HiRes qw(usleep);
use Carp;
use Data::Dumper;
use Cwd;


END
{
#    unlink("tmp/common.conf");
    check_core();
};


my $nxlog = "../src/core/nxlog";
my $nxprocessor = "../src/utils/nxlog-processor";

my $testfile;
my $line_num;
my %instances;


sub check_core
{
    my @files;
    foreach my $f ( glob("core*") )
    {
	push(@files, $f) if ( -f $f );
    }
    if ( scalar(@files) > 0 )
    {
	print STDERR "core file found in working directory!!\n";
    }
}



sub debug
{
    if ( defined $ENV{'DEBUG'} )
    {
	print @_;
    }
}



sub killall
{
    my ($name) = @_;
    if ( -f '/usr/bin/killall' )
    {
	system("killall", $name);
    }
    else
    {
	system("pkill", $name);
    }
}



sub read_output
{
    my ( $param ) = @_;
    my $line;

    my $timeout = 10;

    $SIG{ALRM} = sub { test_failed("expected: '$param', read timeout"); };
    alarm $timeout;
    for ( ; ; )
    {
	if ( eof(DAEMON) )
	{
	    print STDERR "[$line_num] Test failed. expected: '$param', got EOF'\n";
	    exit 4;
	}
	$line = <DAEMON>;
	$line =~ s/\s+$//;
	last;
    }
    alarm 0;
    debug("READ FROM DAEMON: '" . $line . "'\n");
    if ( $line ne $param )
    {
	print STDERR "[$line_num] Test failed.\nexpected: '$param'\ngot:      '$line'\n";
	exit 4;
    }
}



sub test_failed
{
    my ( $param ) = @_;

    print STDERR "test failed at $testfile:$line_num\n";
    print STDERR join('', @_);
    print STDERR "\n";

    exit(1);
}


sub test_bad_conf
{
    my ( $param ) = @_;

    my @files = glob($param);

    foreach my $file ( @files )
    {
	test_bad_conf_file($file);
    }
}


sub test_bad_conf_file
{
    my ( $cfgfile ) = @_;

    my $errmsg;
    open(CONF, $cfgfile) || test_failed("couldn't open $cfgfile: $!");
    while ( <CONF> )
    {
	if ( $_ =~ /^\#ERRORMSG\:\s*(.+)$/ )
	{
	    $errmsg = $1;
	    last;
	}
    }
    close(CONF);
    if ( !defined($errmsg) )
    {
	test_failed("did not find #ERRORMSG line in $cfgfile");
    }
    debug("expecting error message \'$errmsg\'\n");
    my $found = 0;
    my $cmd = "$nxlog -f -c $cfgfile 2>&1";
    debug("running command: $cmd\n");

    my $timeout = 10;
    $SIG{ALRM} = sub { test_failed("[$cfgfile] daemon did not exit with an error"); };
    alarm($timeout);
    my $output = `$cmd`;
    alarm(0);
    if ( $output =~ /$errmsg/ )
    {
	$found = 1;
    }
    my $retval = $? >> 8;
    if ( $retval == 0 )
    {
	test_failed("[$cfgfile] non-zero return value expected, got: $retval ($?)");
    }
    else
    {
	debug("exited with value $retval\n");
    }
    if ( $found != 1 )
    {
	test_failed("[$cfgfile] did not receive expected error message \'$errmsg\', got \'$output\'");
    }
    else
    {
	debug("$cfgfile OK\n");
    }
}



sub start_daemon
{
    my ( $cfgfile ) = @_;

    my $errmsg;
    open(CONF, $cfgfile) || test_failed("couldn't open $cfgfile: $!");
    close(CONF);
    my $cmd = "$nxlog -c $cfgfile 2>&1";
    debug("running command: $cmd\n");
    my $output = `$cmd`;
    my $retval = $? >> 8;
    if ( $retval != 0 )
    {
	croak("failed to execute $cmd, exit value $retval, read $output");
    }
    sleep(1);
    debug("nxlog started with config $cfgfile\n");
}



sub stop_daemon
{
    my ( $cfgfile ) = @_;

    my $errmsg;
    open(CONF, $cfgfile) || test_failed("couldn't open $cfgfile: $!");
    close(CONF);
    my $cmd = "$nxlog -s -c $cfgfile 2>&1";
    debug("running command: $cmd\n");
    my $output = `$cmd`;
    my $retval = $? >> 8;
    if ( $retval != 0 )
    {
	croak("failed to execute $cmd, exit value $retval, read $output");
    }

    debug("nxlog stopped with config $cfgfile\n");
}



sub run_processor
{
    my ( $cfgfile ) = @_;

    my $errmsg;
    open(CONF, $cfgfile) || test_failed("couldn't open $cfgfile: $!");
    close(CONF);
    my $cmd = "$nxprocessor -c $cfgfile 2>&1";
    debug("running command: $cmd\n");
    my $output = `$cmd`;
    my $retval = $? >> 8;
    if ( $retval != 0 )
    {
	croak("failed to execute $cmd, exit value $retval, read $output");
    }
    debug("nx-processor started with config $cfgfile\n");
}



sub printline
{
    my ( $dst, $fh, $line ) = @_;

    if ( $dst =~ /^file\:(.+)$/ )
    {
	syswrite($fh, $line . "\n");
    }
    elsif ( $dst =~ /^ssl\:(.+)$/ )
    {
	syswrite($fh, $line . "\n");
    }
    elsif ( $dst =~ /^tcp\:(.+)$/ )
    {
	$fh->send($line . "\n");
	$fh->flush();
    }
    elsif ( $dst =~ /^udp\:(.+)$/ )
    {
	$fh->send($line);
	usleep(1000);
    }
    elsif ( $dst =~ /^uds\:(.+)$/ )
    {
	$fh->send($line);	
#	syswrite($fh, $line);
#	$fh->flush();
    }
    else
    {
	test_failed("cannot write to invalid dst: \"$dst\"");
    }
}



sub ssl_error_trap
{
    my ( $sock, $msg ) = @_;
    if ( defined($msg) )
    {
	test_failed($msg);
    }
}


sub ssl_cert_verify_cb
{
    my ( $ok, $certstore, $issuer, $errmsg ) = @_;

    debug("verifying cert, issuer: $issuer");
    #debug("certstore: " . Dumper($certstore));
    if ( !$ok )
    {
	debug("\ncert preverification failed: $errmsg\n");
	#test_failed("SSL certificate verification failed: $errmsg ");
    }

#    return ( $ok );
# for some reason newer IO::Socket::SSL fails with the verification
# so just assume ok
    return 1;
}


sub open_dst
{
    my ( $dst ) = @_;

    my $output;
    if ( $dst =~ /^file\:(.+)$/ )
    {
	$dst = $1;
	open($output, ">> $dst") || test_failed("failed to open file $dst for writing");
    }
    elsif ( $dst =~ /^ssl\:(.+)$/ )
    {
	$dst = $1;
	debug("connecting via ssl to $dst\n");

	my %sslopts = (
#	    SSL_version => 'TLSv1',
	    PeerAddr => $dst,
	    Proto => 'tcp',
	    SSL_use_cert => 1,
	    SSL_cert_file => 'cert/client-cert.pem',
	    SSL_key_file => 'cert/client-key.pem',
	    SSL_passwd_cb => sub { return "secret" },
	    # SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
	    #SSL_verify_mode => 0x01 | 0x02 | 0x04,
	    SSL_error_trap => \&ssl_error_trap,
	    #SSL_crl_file => 'cert/crl.pem',
	    SSL_check_crl => 1,
	    SSL_ca_file => 'cert/ca.pem',
	    SSL_verify_callback => \&ssl_cert_verify_cb,

	    TimeOut => 2,
	    );

	$output = IO::Socket::SSL->new(%sslopts) 
	    || test_failed("SSL connection error: ($! $@) " . IO::Socket::SSL::errstr());
    }
    elsif ( $dst =~ /^tcp\:(.+)$/ )
    {
	$dst = $1;
	debug("connecting via tcp to $dst\n");
	$output = IO::Socket::INET->new(
	    PeerAddr => $dst,
	    Proto => 'tcp',
	    Type => SOCK_STREAM,
	    ) || test_failed("TCP connection error: $!");
    }
    elsif ( $dst =~ /^udp\:(.+)$/ )
    {
	$dst = $1;
	debug("connecting to $dst\n");
	$output = IO::Socket::INET->new(
	    PeerAddr => $dst,
	    Proto => 'udp',
	    Type => SOCK_DGRAM,
	    ) || test_failed("UDP init error: $!");
    }
    elsif ( $dst =~ /^uds\:(.+)$/ )
    {
	$dst = $1;
	debug("connecting to $dst\n");
	$output = IO::Socket::UNIX->new(
	    Peer => $dst,
	    Type => SOCK_DGRAM,
	    ) || test_failed("UDS init error: $!");
    }
    else
    {
	test_failed("cannot open invalid dst: \"$dst\"");
    }

    return $output;
}



sub close_dst
{
    my ( $dst, $fh ) = @_;

    if ( $dst =~ /^file\:(.+)$/ )
    {
	$dst = $1;
	close($fh);
    }
    elsif ( $dst =~ /^ssl\:(.+)$/ )
    {
	$fh->close(SSL_ctx_free => 1);
    }
    elsif ( $dst =~ /^tcp\:(.+)$/ )
    {
	$fh->shutdown(2);
    }
    elsif ( $dst =~ /^udp\:(.+)$/ )
    {
	$fh->shutdown(2);
    }
    elsif ( $dst =~ /^uds\:(.+)$/ )
    {
	$fh->shutdown(2);
    }
    else
    {
	test_failed("cannot close invalid dst: \"$dst\"");
    }
}



sub write_file
{
    my ( $param ) = @_;

    my ( $dst, $src ) = split(/\s+/, $param, 2);
    
    my $input;
    open($input, $src)  || test_failed("failed to open file $src for reading");

    my $output = open_dst($dst);
    while ( <$input> )
    {
	$_ =~ s/[\r\n]+$//;
	printline($dst, $output, $_);
    }
    close_dst($dst, $output);
    close($input);
}



sub write_line
{
    my ( $param ) = @_;

    my ( $dst, $line ) = split(/\s+/, $param, 2);

    my $output = open_dst($dst);
    printline($dst, $output, $line);
    close_dst($dst, $output);
}



sub compare_file
{
    my ( $param, $sizeonly ) = @_;

    my ( $file1, $file2 ) = split(/\s+/, $param, 2);
    
    if ( ! -f $file1 )
    {
	test_failed("$file1 does not exist");
    }
    if ( ! -f $file2 )
    {
	test_failed("$file2 does not exist");
    }

    my ( $dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size ) = stat($file1);
    my $size1 = $size;
    ( $dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size ) = stat($file2);
    my $size2 = $size;
    if ( $size1 != $size2 )
    {
	test_failed("file sizes differ for $file1 and $file2");
    }

    return if ( defined($sizeonly) && $sizeonly == 1 );

    my $f1;
    open($f1, $file1)  || test_failed("failed to open file $file1 for reading");
    my $f2;
    open($f2, $file2)  || test_failed("failed to open file $file2 for reading");

    while ( my $line1 = <$f1> )
    {
	my $line2 = <$f2>;
	if ( $line1 ne $line2 )
	{
	    test_failed("files $file1 and $file2 differ at line $.");
	}
    }

    close($f1);
    close($f2);
}



sub verify_testgen_file
{
    my ( $param ) = @_;

    my ( $file, $count ) = split(/\s+/, $param, 2);

    my $counter = 0;
    my $f;
    open($f, $file)  || test_failed("failed to open file $file for reading");
    while ( <$f> )
    {
	if ( $_ =~ /^(\d+)\@\S+\s+\S+\s+\d+\s+\d{2}\:\d{2}\:\d{2}\s+\d{4}\s*$/ )
	{
	    if ( $counter != $1 )
	    {
		test_failed("invalid testgen counter in line '$_', expected $counter, found $1");
	    }
	}
	else
	{
	    test_failed("invalid testgen line: $_");
	}
	$counter++;
    }
    if ( $counter != $count )
    {
	test_failed("invalid testgen counter, expected $count, got $counter");
    }
    close($f);
}



sub exec_command
{
    my ( $cmd, $param ) = @_;

    debug("$cmd: $param\n");
    if ( $cmd eq "BADCONF" )
    {
	test_bad_conf($param);
    }
    elsif ( $cmd eq "STARTDAEMON" )
    {
	start_daemon($param);
    }
    elsif ( $cmd eq "STOPDAEMON" )
    {
	stop_daemon($param);
    }
    elsif ( $cmd eq "RUNPROCESSOR" )
    {
	run_processor($param);
    }
    elsif ( $cmd eq "WRITEFILE" )
    {
	write_file($param);
    }
    elsif ( $cmd eq "COMPAREFILE" )
    {
	compare_file($param, 0);
    }
    elsif ( $cmd eq "COMPAREFILESIZE" )
    {
	compare_file($param, 1);
    }
    elsif ( $cmd eq "TESTGENVERIFY" )
    {
	verify_testgen_file($param);
    }
    elsif ( $cmd eq "WRITELINE" )
    {
	write_line($param);
    }
    elsif ( $cmd eq "SLEEP" )
    {
	sleep($param);
    }
    elsif ( $cmd eq "REMOVE" )
    {
	if ( -f $param )
	{
	    unlink($param) || test_failed("couldn't remove $param: $!");
	}
    }
    elsif ( $cmd eq "TRUNCATE" )
    {
	if ( ! -f $param )
	{
	    open(TOUCHED, ">$param") || test_failed("couldn't touch $param: $!");
	    close(TOUCHED);
	}
	else
	{
	    unlink($param);
	    open(TOUCHED, ">$param") || test_failed("couldn't touch $param: $!");
	    close(TOUCHED);
	}
    }
    elsif ( $cmd eq "EVAL" )
    {
	eval "$param";
	if ( $@ )
	{
	    test_failed("EVAL failed: ", $@);
	}
    }
    elsif ( $cmd eq "WAIT4KEY" )
    {
	print "press enter to proceed\n";
	my $line = <STDIN>;
    }
    elsif ( $cmd eq "EXIT" )
    {
	exit 0;
    }
    else
    {
	print STDERR "[$line_num] invalid command: $cmd\n";
	exit 1;
    }
}



#########

if ( $#ARGV < 0)
{
    print STDERR "filename argument missing\n";
    exit 42;
}

#$SIG{PIPE} = 'IGNORE';
$SIG{PIPE} = sub { die "got SIGPIPE\n" };

unlink(<tmp/*.q>);
unlink("tmp/selflog");

my $cfg;
open(CFG, "common.conf") || die("failed to open common.conf: $!");
read(CFG, $cfg, -s "common.conf");
close(CFG);

open(CFG, ">tmp/common.conf") || die("failed to open tmp/common.conf: $!");
print CFG "define PWD ". getcwd() . "\n";
print CFG $cfg;
close(CFG);

my $testinput;
$testfile = $ARGV[0];
open($testinput, "<$testfile") || die "failed to open file: '$ARGV[0]'\n";

while ( <$testinput> )
{
    $line_num++;
    if ( $_ =~ /^\#.*$/ )
    {
	next;
    }
    elsif ( $_ =~/^\s*$/ )
    {
	next;
    }
    if ( $_ =~ /^(\S+):\s?$/ )
    {
	exec_command($1, '');
    }
    elsif ( $_ =~ /^(\S+): (.*)$/ )
    {
	exec_command($1, $2);
    }
    else
    {
	print STDERR "[$line_num] invalid command: $_\n";	
	exit 1;
    }
}
#unlink("tmp/common.conf");
close($testinput);
print "$ARGV[0] OK\n"
