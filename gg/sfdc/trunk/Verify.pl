
### Class Verify: Verify SFD info #################################################

BEGIN {
    package Verify;

    sub new {
	my $proto    = shift;
	my %params   = @_;
	my $class    = ref($proto) || $proto;
	my $self     = {};
	$self->{SFD} = $params{'sfd'};
	$self->{CNT} = 0;
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "Checking SFD for $$sfd{'libname'} ...";
	$self->{CNT} = 0;
    }

    sub function {
	my $self = shift;
	my $sfd  = $self->{SFD};

	# Well ... the file is already parsed, so just pretend to
	# check it. :-)

	++$self->{CNT};
    }

    sub footer {
	my $self = shift;
	my $sfd  = $self->{SFD};

	printf "$self->{CNT} function%s verified\n", $self->{CNT} == 1 ? "" : "s";
    }
}
