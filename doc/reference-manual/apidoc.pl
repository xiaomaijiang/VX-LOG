#!/usr/bin/perl -w
#########################################################################
# This program generates AsciiDoc documentation from the -apidoc.xml file
#########################################################################
use 5.008000;
use open OUT => ':utf8';
use strict;
use warnings;

use Carp;
use Data::Dumper;
use XML::Simple;

if ( scalar(@ARGV) < 1 )
{
    print STDERR "usage: apidoc.pl [-lang LANG] api.xml\n";
    exit(1);
}
my $input = shift(@ARGV);
my $lang = "en";
if ( $input eq "-lang" )
{
    $lang = shift(@ARGV);
    $input = shift(@ARGV);
}
my $xs = XML::Simple->new();
my $hash = $xs->XMLin($input,
		      KeyAttr => [],
		      KeepRoot => 0,
		      ForceArray => 0,
    ) || croak("XMLin failed: " . $@);

my $module = $hash->{module};

open(OUT, "> apidoc-" . $module . ".adoc") ||
    croak("couldn't open output");

dump_functions($hash);
dump_procedures($hash);
close(OUT);

exit 0;
########################################################################

sub dump_args
{
    my ( $args ) = @_;

    if ( !defined($args) )
    {
	return;
    }

    if ( ref($args) ne 'ARRAY' )
    {
	$args = [$args];
    }

    foreach my $arg ( @{$args} )
    {
	print OUT "* `" . $arg->{name} . "` type: <<lang_type_" .
                  $arg->{type} . "," . $arg->{type} . ">>\n";
    }
}


sub get_args
{
    my ( $args ) = @_;

    if ( !defined($args) )
    {
	return "()";
    }

    if ( ref($args) ne 'ARRAY' )
    {
	$args = [$args];
    }

    my $retval = "(";

    foreach my $arg ( @{$args} )
    {
    $retval .= "<<lang_type_" . $arg->{type} . "," . $arg->{type} . ">> " .
               $arg->{name} . ", ";
    }
    chop($retval);
    chop($retval);

    $retval .= ")";

    return $retval;
}



sub dump_functions
{
    my $hash = shift;

    my $funcs = $hash->{function};
    if ( defined($funcs) && (ref($funcs) ne 'ARRAY') )
    {
	$funcs = [$hash->{function}];
    }

    if ( defined($funcs) && (@{$funcs} > 0) )
    {
	print OUT "[[" . $module . "_funcs]]\n" .
              "===== Functions\n\n" .
              "The following functions are exported by $module.\n\n";

	my %defines;

	# Sort functions
	@$funcs = sort { lc $a->{name} cmp lc $b->{name} } @$funcs;

	foreach my $func ( @{$funcs} )
	{
	    my $desc;
	    if ( defined($func->{description}{$lang}) )
	    {
		$desc = $func->{description}{$lang};
	    }
	    else
	    {
		$desc = $func->{description}{en};
	    }
	    if ( ! defined($defines{$func->{name}}) )
	    {
		print OUT "[[$module" . "_func_" . $func->{name} .
                          "]]\n";
		$defines{$func->{name}} = 1;
	    }
        print OUT "<<lang_type_" . $func->{rettype} . ",$func->{rettype}>>" .
	          " `" . $func->{name} . get_args($func->{arg}) .
	          "`::\n+\n--\n" .
	          $desc . "\n--\n\n";
	}
	print OUT "\n";
    }
    else
    { # no functions
    }
}



sub dump_procedures
{
    my $hash = shift;

    my $procs = $hash->{procedure};
    if ( defined($procs) && (ref($procs) ne 'ARRAY') )
    {
	$procs = [$hash->{procedure}];
    }

    if ( defined($procs) && (@{$procs} > 0) )
    {
	print OUT "[[" . $module . "_procs]]\n" .
	          "===== Procedures\n\n" .
              "The following procedures are exported by $module.\n\n";

	my %defines;

	# Sort procedures
	@$procs = sort { lc $a->{name} cmp lc $b->{name} } @$procs;

	foreach my $proc ( @{$procs} )
	{
	    my $desc;
	    if ( defined($proc->{description}{$lang}) )
	    {
		$desc = $proc->{description}{$lang};
	    }
	    else
	    {
		$desc = $proc->{description}{en};
	    }
	    if ( ! defined($defines{$proc->{name}}) )
	    {
		print OUT "[[$module" . "_proc_" . $proc->{name} .
                          "]]\n";
		$defines{$proc->{name}} = 1;
	    }
	    print OUT "`" . $proc->{name} . get_args($proc->{arg}) .
                      ";`::\n+\n--\n" . $desc . "\n--\n";
	    print OUT "\n";
	}
    }
    else
    { # no procedures
    }
}
