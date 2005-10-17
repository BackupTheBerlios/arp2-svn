
### Class MacroAOS4: Create a AOS4-style macro file ###########################

BEGIN {
    package MacroAOS4;
    use vars qw(@ISA);
    @ISA = qw( Macro );

    sub new {
	my $proto  = shift;
	my $class  = ref($proto) || $proto;
	my $self   = $class->SUPER::new( @_ );
	my $sfd    = $self->{SFD};
	$self->{CALLBASE} = "I$sfd->{BaseName}";
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	my $sfd  = $self->{SFD};

	$self->SUPER::header (@_);
	
	if ($$sfd{'base'} ne '') {
	    print "#ifndef $self->{BASE}\n";
	    print "#define $self->{BASE} I$sfd->{BaseName}\n";
	    print "#endif /* !$self->{BASE} */\n";
	    print "\n";
	}
    }

    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if ($prototype->{type} eq 'function' ||
	    $prototype->{type} eq 'varargs') {
	    printf "	(((struct $sfd->{BaseName}IFace *)(_base))->$prototype->{funcname})(";
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

	if ($prototype->{type} eq 'function' ||
	    $prototype->{type} eq 'varargs') {
	    print ", " unless $argnum == 0;
	    if ($argname ne '...') {
		print "$argname";
	    }
	    else {
		print "## __VA_ARGS__";
	    }
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

	
	if ($prototype->{type} eq 'function' ||
	    $prototype->{type} eq 'varargs') {
	    print ")\n";
	}
	else {
	    $self->SUPER::function_end (@_);
	}
    }
}
