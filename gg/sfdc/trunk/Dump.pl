
### Class Dump: Dump SFD info ##################################################

BEGIN {
    package Dump;

    sub new {
	my $proto    = shift;
	my %params   = @_;
	my $class    = ref($proto) || $proto;
	my $self     = {};
	$self->{SFD} = $params{'sfd'};
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "SFD information\n";
	print "ŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻ\n";
	print "Copyright:		$$sfd{'copyright'}\n";
	print "RCS ID:			$$sfd{'id'}\n";
	print "Module name:		$$sfd{'libname'}\n";
	print "Module base:		$$sfd{'base'}\n";
	print "Module base type:	$$sfd{'basetype'}\n";
	print "Module base names:	$$sfd{'basename'}, $$sfd{'BASENAME'}, ";
	print "$$sfd{'Basename'}\n";
	print "\n";
	print "Include files:		";
	print join ("\n			", @{$$sfd{'includes'}});
	print "\n";
	print "\n";
    }

    sub function {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	print "* Line $$prototype{'line'}: $$prototype{'funcname'}()\n";
	print "	Type:			" . ucfirst $$prototype{'type'} . "\n";
	if ($$prototype{'type'} ne 'function') {
	    print "	Real function name:\t$$prototype{'real_funcname'}\n";
	}
	print "	Visibility:		";
	print $$prototype{'private'} == 0 ? "Public\n" : "Private\n";
	print "	Library offset/bias:	-$$prototype{'bias'}\n";
	print "	Available since:	V$$prototype{'version'}\n";
	print "	Comment:		$$prototype{'comment'}\n";
	print "\n";
	print "	Return value:		$$prototype{'return'}\n";
	print "	Argument names:		";
	print join (", ", @{$$prototype{'argnames'}});
	print "\n";
	print "	Argument types:		";
	print join (",\n\t\t\t\t", @{$$prototype{'argtypes'}});
	print "\n";

	print "\n";

#		value   => $proto_line,

#    $$prototype{'return'}     = $return;
#    $$prototype{'funcname'}   = $name;
#    @{$$prototype{'args'}}     = ();
#    @{$$prototype{'regs'}} = split(/,/,lc $registers);  # Make regs lower case
#    @{$$prototype{'argnames'}} = ();                    # Initialize array
#    @{$$prototype{'argtypes'}} = ();                    # Initialize array
    }

    sub footer {
	print "\n";
    }
}
