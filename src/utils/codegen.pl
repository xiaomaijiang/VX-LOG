#!/usr/bin/perl -w
###############################################################################
# This program creates C struct definitions of functions/procedures defined
# in an module declaration xml file
###############################################################################
use 5.008000;
use strict;
use warnings;

use Carp;
use Data::Dumper;
use XML::Simple;

if ( scalar(@ARGV) != 1 )
{
    print STDERR "INPUT filename argument required\n";
    exit(1);
}
my $input = $ARGV[0];
my $xs = XML::Simple->new();
my $hash = $xs->XMLin($input,
		      KeyAttr => [],
		      KeepRoot => 0,  #strips the <fields> tag without KeepRoot
		      ForceArray => 0,
    ) || croak("XMLin failed: " . $@);

my $module = $hash->{module};

open(COUT, "> expr-" . $module . "-funcproc.c") || croak("couldn't open .c output");
open(HOUT, "> expr-" . $module . "-funcproc.h") || croak("couldn't open .h output");

print COUT "/* Automatically generated from $input by codegen.pl */\n";
print HOUT "/* Automatically generated from $input by codegen.pl */\n";
print HOUT "#ifndef __NX_EXPR_FUNCPROC_" . $module . "_H\n";
print HOUT "#define __NX_EXPR_FUNCPROC_" . $module . "_H\n\n";

my $num_func = 0;
my $num_proc = 0;
dump_includes($hash);
dump_functions($hash);
dump_procedures($hash);
dump_exports($hash);

print HOUT "\n\n#endif /* __NX_EXPR_FUNCPROC_" . $module . "_H */\n";

close(COUT);
close(HOUT);

exit 0;
###########################################################################################################

sub dump_exports
{
    my $hash = shift;

    my $pref = get_prefix($hash);
    print COUT "nx_module_exports_t nx_module_exports_" . $module . " = {\n";
    print COUT "	$num_func,\n";
    if ( $num_func > 0 )
    {
	print COUT "	$pref" . "funcs,\n";
    }
    else
    {
	print COUT "	NULL,\n";
    }
    print COUT "	$num_proc,\n";
    if ( $num_proc > 0 )
    {
	print COUT "	$pref" . "procs,\n";
    }
    else
    {
	print COUT "	NULL,\n";
    }
    print COUT "};\n";
}



sub dump_includes
{
    my $hash = shift;

    my $incs = $hash->{include};
    if ( ref($incs) ne 'ARRAY' )
    {
	$incs = [$hash->{include}];
    }
    foreach my $inc ( @{$incs} )
    {
	print HOUT "#include \"$inc\"\n";
    }
    print COUT "#include \"expr-$hash->{module}-funcproc.h\"\n";
    print COUT "\n";
    print HOUT "\n";
}



sub get_cbname
{
    my ( $args, $cb ) = @_;

    croak("cb tag missing") unless ( defined($cb) );

    if ( ref($args) ne 'ARRAY' )
    {
	$args = [$args];
    }

    my $retval = $cb;

    foreach my $arg ( @{$args} )
    {
	$retval .= '_' . $arg->{type};
    }

    return ( $retval );
}



sub get_argcnt
{
    my ( $args ) = @_;

    return 0 unless ( defined($args) );

    if ( ref($args) ne 'ARRAY' )
    {
	$args = [$args];
    }
    my $argcnt = scalar(@{$args});

    return ( $argcnt );
}



sub dump_args
{
    my ( $args, $cb ) = @_;

    if ( !defined($args) )
    {
	return;
    }

    if ( ref($args) ne 'ARRAY' )
    {
	$args = [$args];
    }

    print COUT "const char *" . get_cbname($args, $cb) . "_argnames[] = {\n    ";
    foreach my $arg ( @{$args} )
    {
	print COUT "\"$arg->{name}\", ";
    }
    print COUT "\n};\n";
    
    print COUT "nx_value_type_t " . get_cbname($args, $cb) . "_argtypes[] = {\n    ";
    foreach my $arg ( @{$args} )
    {
	my $argtype = get_value_type($arg->{type});
	print COUT "$argtype, ";
    }
    print COUT "\n};\n";
}



sub dump_functions
{
    my $hash = shift;

    my $funcs = $hash->{function};
    if ( defined($funcs) && (ref($funcs) ne 'ARRAY') )
    {
	$funcs = [$hash->{function}];
    }
    my $prefix = get_prefix($hash);

    if ( defined($funcs) && (@{$funcs} > 0) )
    {
	print COUT "\n/* FUNCTIONS */\n\n";
	print HOUT "\n/* FUNCTIONS */\n\n";

	print HOUT "#define " . $prefix . "func_num ". scalar(@{$funcs}) . "\n";
	$num_func = scalar(@{$funcs});

	my %decls;
	foreach my $func ( @{$funcs} )
	{
	    print COUT "// $func->{name}\n";
	    if ( !defined($decls{$func->{name} . '_' .  $func->{cb}}) )
	    {
		print HOUT "void $func->{cb}(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_value_t *retval, int32_t num_arg, nx_value_t *args);\n";
		$decls{$func->{name} . '_' .  $func->{cb}} = 1;
	    }
	    dump_args($func->{arg}, $func->{cb});
	}
	print COUT "\n";
	print COUT "nx_expr_func_t " . $prefix . "funcs[] = {\n";
	foreach my $func ( @{$funcs} )
	{
	    if ( !defined($func->{type}) )
	    {
		$func->{type} = 'global';
	    }
	    my $type = get_type($func->{type});
	    my $rettype = get_value_type($func->{rettype});
	    print COUT 
" {
   { .next = NULL, .prev = NULL },
   NULL,
   \"$func->{name}\",
   $type,
   $func->{cb},
   $rettype,
";
	    if ( defined($func->{arg}) )
	    {
		print COUT "   " . get_argcnt($func->{arg}) . ",\n";
		print COUT "   " . get_cbname($func->{arg}, $func->{cb}) . "_argnames,\n";
		print COUT "   " . get_cbname($func->{arg}, $func->{cb}) . "_argtypes,\n";
	    }
	    else
	    {
		print COUT "   0,\n";
		print COUT "   NULL,\n";
		print COUT "   NULL,\n";
	    }
	    print COUT " },\n";
	}
	print COUT "};\n\n";
    }
    else
    { # no functions
	print HOUT "#define " . $prefix . "func_num 0\n";
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
    my $prefix = get_prefix($hash);

    if ( defined($procs) && (@{$procs} > 0) )
    {
	print COUT "\n/* PROCEDURES */\n\n";
	print HOUT "\n/* PROCEDURES */\n\n";

	print HOUT "#define " . $prefix . "proc_num ". scalar(@{$procs}) . "\n";
	$num_proc = scalar(@{$procs});

	my %decls;
	foreach my $proc ( @{$procs} )
	{
	    print COUT "// $proc->{name}\n";
	    if ( !defined($decls{$proc->{name} . '_' .  $proc->{cb}}) )
	    {
		print HOUT "void $proc->{cb}(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);\n";
		$decls{$proc->{name} . '_' .  $proc->{cb}} = 1;
	    }

	    dump_args($proc->{arg}, $proc->{cb});
	}
	print COUT "\n";
	print COUT "nx_expr_proc_t " . $prefix . "procs[] = {\n";
	foreach my $proc ( @{$procs} )
	{
	    if ( !defined($proc->{type}) )
	    {
		$proc->{type} = 'global';
	    }
	    my $type = get_type($proc->{type});
	    print COUT 
" {
   { .next = NULL, .prev = NULL },
   NULL,
   \"$proc->{name}\",
   $type,
   $proc->{cb},
";
	    if ( defined($proc->{arg}) )
	    {
		print COUT "   " . get_argcnt($proc->{arg}) . ",\n";
		print COUT "   " . get_cbname($proc->{arg}, $proc->{cb}) . "_argnames,\n";
		print COUT "   " . get_cbname($proc->{arg}, $proc->{cb}) . "_argtypes,\n";
	    }
	    else
	    {
		print COUT "   0,\n";
		print COUT "   NULL,\n";
		print COUT "   NULL,\n";
	    }
	    print COUT " },\n";
	}
	print COUT "};\n\n";
    }
    else
    { # no procedures
	print HOUT "#define " . $prefix . "proc_num 0\n";
    }
}



sub get_prefix
{
    my $hash = shift;

    my $prefix = 'nx_api_declarations_';
    
    if ( defined($hash->{module}) )
    {
	$prefix = 'nx_api_declarations_' . $hash->{module} . '_';
	$prefix =~ s/\-/_/g;
    }

    return ( $prefix );
}



sub get_type
{
    my ( $str ) = @_;

    if ( lc($str) eq 'global' )
    {
	return ( 'NX_EXPR_FUNCPROC_TYPE_GLOBAL' );
    }
    elsif ( lc($str) eq 'private' )
    {
	return ( 'NX_EXPR_FUNCPROC_TYPE_PRIVATE' );
    }
    elsif ( lc($str) eq 'public' )
    {
	return ( 'NX_EXPR_FUNCPROC_TYPE_PUBLIC' );
    }

    croak("invalid type: $str");
}



sub get_value_type
{
    my ( $type ) = @_;

    if ( ! defined($type) )
    {
	croak("type required");
    }
    if ( $type eq 'integer' )
    {
	return ( 'NX_VALUE_TYPE_INTEGER' );
    }
    elsif ( $type eq 'string' )
    {
	return ( 'NX_VALUE_TYPE_STRING' );
    }
    elsif ( $type eq 'boolean' )
    {
	return ( 'NX_VALUE_TYPE_BOOLEAN' );
    }
    elsif ( $type eq 'ip4addr' )
    {
	return ( 'NX_VALUE_TYPE_IP4ADDR' );
    }
    elsif ( $type eq 'ip6addr' )
    {
	return ( 'NX_VALUE_TYPE_IP6ADDR' );
    }
    elsif ( $type eq 'binary' )
    {
	return ( 'NX_VALUE_TYPE_BINARY' );
    }
    elsif ( $type eq 'datetime' )
    {
	return ( 'NX_VALUE_TYPE_DATETIME' );
    }
    elsif ( $type eq 'unknown' )
    {
	return ( 'NX_VALUE_TYPE_UNKNOWN' );
    }
    elsif ( $type eq 'varargs' )
    {
	return ( 'NX_EXPR_VALUE_TYPE_VARARGS' );
    }
    else
    {
	croak("invalid field type $type");
    }
}
