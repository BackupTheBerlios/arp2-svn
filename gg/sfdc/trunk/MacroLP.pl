
### Class MacroLP: Create a LP-style macro file ###############################

BEGIN {
    package MacroLP;
    use vars qw(@ISA);
    @ISA = qw( Macro );

    sub new {
	my $proto  = shift;
	my $class  = ref($proto) || $proto;
	my $self   = $class->SUPER::new( @_ );
	bless ($self, $class);
	return $self;
    }

    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if ($$prototype{'type'} ne 'varargs') {
	    printf "	LP%d%s%s(0x%x, ", $$prototype{'numargs'},
	    $prototype->{nr} ? "NR" : "",
	    $prototype->{nb} ? "NB" : "", $$prototype{'bias'};

	    if (!$prototype->{nr}) {
		print "$$prototype{'return'}, ";
	    }

	    print "$$prototype{'funcname'}";
	}
	else {
	    $self->SUPER::function_start (@_);
	}
    }

    sub function_arg {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $argtype   = $params{'argtype'};
	my $argname   = $params{'argname'};
	my $argreg    = $params{'argreg'};
	my $argnum    = $params{'argnum'};
	my $sfd       = $self->{SFD};

	if ($$prototype{'type'} ne 'varargs') {
	    print ", $argtype, $argname, $argreg";
	}
	else {
	    $self->SUPER::function_arg (@_);
	}
    }
    
    sub function_end {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	
	if ($$prototype{'type'} ne 'varargs') {

	    if (!$prototype->{nb}) {
		print ",\\\n	, $self->{BASE})\n";
	    }
	    else {
		print ")\n";
	    }
	}
	else {
	    $self->SUPER::function_end (@_);
	}
    }
}
