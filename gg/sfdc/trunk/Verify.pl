
### Class Verify: Verify SFD info #################################################

BEGIN {
    package Verify;

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
	print "Checking file ...";
    }

    sub function {
    }

    sub footer {
	print "OK\n";
    }
}
