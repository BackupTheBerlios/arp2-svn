
### Class Macro68k: Implements m68k-only features for macro files ##############

BEGIN {
    package Macro68k;
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
	my $nr        = $$prototype{'return'} =~ /^(VOID|void)$/;
	my $nb        = $$sfd{'base'} eq '';

	if ($$prototype{'type'} !~ /^(varargs|stdarg)$/) {

	    my $regs      = join(',', @{$$prototype{'regs'}});
	    my $argtypes  = join(',', @{$$prototype{'argtypes'}});
	    my $a4        = $regs =~ /a4/;
	    my $a5        = $regs =~ /a5/;
	    my $fp        = $argtypes =~ /\(\*\)/;

	    $self->{FUNCARGTYPE} = '';
	    for my $argtype (@{$$prototype{'argtypes'}}) {
		if ($argtype =~ /\(\*\)/) {
		    $self->{FUNCARGTYPE} = $argtype;
		    last;
		}
	    }
	
	    printf "	LP%d%s%s%s%s%s(0x%x, ", $#{$$prototype{'args'}} + 1,
	    $a4 ? "A4" : "", $a5 ? "A5" : "",
	    $self->{FUNCARGTYPE} ne '' ? "FP" : "",
	    $nr ? "NR" : "", $nb ? "NB" : "", $$prototype{'bias'};

	    if (!$nr) {
		print "$$prototype{'return'}, ";
	    }

	    print "$$prototype{'funcname'}, ";
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
	my $sfd       = $self->{SFD};

	if ($$prototype{'type'} !~ /^(varargs|stdarg)$/ &&
	    $argtype =~ /\(\*\)/) {
	    
	    print "__fpt, $argname, $argreg, ";
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

	if ($$prototype{'type'} !~ /^(varargs|stdarg)$/) {
	    if( $$sfd{'base'} ne '') {
		print "\\\n	, $self->{BASE}";
	    }

	    if ($self->{FUNCARGTYPE} ne '') {
		my $fa = $self->{FUNCARGTYPE};

		$fa =~ s/\(\*\)/(*__fpt)/;
		
		print ", $fa";
	    }
	    
	    print ")\n";
	}
	else {
	    $self->SUPER::function_end (@_);
	}
    }
}
