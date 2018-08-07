#!/usr/bin/perl -w
#########################################################################
# This program generates AsciiDoc documentation from the -fields.xml file
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

open(OUT, "> fields-" . $module . ".adoc") ||
    croak("couldn't open output");

my $section_level = "=";
if ( $module eq "core" )
{
    $section_level = "";
}

print OUT "[[" . $module . "_fields]]\n" . $section_level .
"==== Fields\n\n" .
"The following fields are used by $module.\n";

my $extra = $hash->{extra}{$lang};
if ( ! defined($extra) )
{
    my $extra = $hash->{extra}{en};
}
if ( $extra )
{
    $extra =~ s/\s+$//;
    $extra =~ s/^\s+//;
    print OUT "\n$extra\n"
}

dump_fields($hash);
close(OUT);

exit 0;
########################################################################

sub dump_fields
{
    my $hash = shift;

    my $fields = $hash->{field};
    if ( defined($fields) && (ref($fields) ne 'ARRAY') )
    {
	$fields = [$hash->{field}];
    }

    if ( defined($fields) && (@{$fields} > 0) )
    {
	print OUT "\n";

	my %defines;

	# Sort fields, with $raw_event field first
	@$fields = sort {
	    my ($x,$y) = (0,0);
	    $x = -1 if $a->{name} eq 'raw_event';
	    $y = -1 if $b->{name} eq 'raw_event';
	    $x <=> $y or
		lc $a->{name} cmp lc $b->{name};
	} @$fields;

	foreach my $field ( @{$fields} )
	{
	    my $desc;
	    if ( defined($field->{description}{$lang}) )
	    {
		$desc = $field->{description}{$lang};
	    }
	    else
	    {
		$desc = $field->{description}{en};
	    }
	    $desc =~ s/\s+$//;
	    $desc =~ s/^\s+//;
	    my $linkname = $field->{name};
	    $linkname =~ s/\./_/g;
	    if ( ! defined($defines{$linkname}) )
	    {
		print OUT "[[$module" . "_field_" . $linkname . "]]\n";
		$defines{$linkname} = 1;
	    }
	    print OUT "`\$" . $field->{name} . "` (type: <<lang_type_" .
		      $field->{type} . "," . $field->{type} . ">>)::\n" .
		      "+\n--\n" . $desc . "\n--\n\n";
	}
    }
    else
    { # no fields
    }
}
