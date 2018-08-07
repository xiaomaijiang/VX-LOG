# FindBin is for adding a path to @INC, this not needed normally
use FindBin;
use lib "$FindBin::Bin/../../../../src/modules/extension/perl";

use strict;
use warnings;

# Without Log::Nxlog you cannot access (read or modify) the event data
use Log::Nxlog;

use Geo::IP;

my $geoip;

BEGIN
{
    # This will be called once when nxlog starts so you can use this to
    # initialize stuff here
    $geoip = Geo::IP->new(GEOIP_MEMORY_CACHE);
}

# This is the method which is invoked from 'Exec' for each event
sub process
{
    # The event data is passed here when this method is invoked by the module
    my ( $event ) = @_;

    # We look up the county of the sender of the message
    my $msgsrcaddr = Log::Nxlog::get_field($event, 'MessageSourceAddress');
    if ( defined($msgsrcaddr) )
    {
	my $country = $geoip->country_code_by_addr($msgsrcaddr);
	$country = "unknown" unless ( defined($country) );
	Log::Nxlog::set_field_string($event, 'MessageSourceCountry', $country);
    }

    # Iterate over the fields
    foreach my $fname ( @{Log::Nxlog::field_names($event)} )
    {
	# Delete all fields except these
	if ( ! (($fname eq 'raw_event') ||
		($fname eq 'AccountName') ||
		($fname eq 'MessageSourceCountry')) )
	{
	    Log::Nxlog::delete_field($event, $fname);
	}
    }

    # Check a field and rename it if it matches
    my $accountname = Log::Nxlog::get_field($event, 'AccountName');
    if ( defined($accountname) && ($accountname eq 'John') )
    {
	Log::Nxlog::set_field_string($event, 'AccountName', 'johnny');
	Log::Nxlog::log_info('renamed john');
    }
}
