#!/usr/bin/perl

use strict;
use warnings;

foreach my $inc (@INC) {
    my $typemap = "$inc/ExtUtils/typemap";
    if ( -e $typemap )
    {
	print $typemap;
	last;
    }
}
