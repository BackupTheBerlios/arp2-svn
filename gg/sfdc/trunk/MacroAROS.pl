
### Class MacroAROS: Implements AROS macro files ###############################

BEGIN {
    package MacroAROS;
    use vars qw(@ISA);
    @ISA = qw( Macro );

    sub new {
	my $proto  = shift;
	my $class  = ref($proto) || $proto;
	my $self   = $class->SUPER::new( @_ );
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "/* Automatically generated header! Do not edit! */\n";
	print "\n";
	print "#ifndef _INLINE_$$sfd{'BASENAME'}_H\n";
	print "#define _INLINE_$$sfd{'BASENAME'}_H\n";
	print "\n";
	print "#ifndef AROS_LIBCALL_H\n";
	print "#include <aros/libcall.h>\n";
	print "#endif /* !AROS_LIBCALL_H */\n";
	print "\n";

	if ($$sfd{'base'} ne '') {
	    print "#ifndef $self->{BASE}\n";
	    print "#define $self->{BASE} $$sfd{'base'}\n";
	    print "#endif /* !$self->{BASE} */\n";
	    print "\n";
	}
    }

    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};
	my $nb        = $$sfd{'base'} eq '';

	if ($$prototype{'type'} !~ /^(varargs|stdarg)$/) {
	    printf "	AROS_LC%d%s(%s, %s, \\\n",
	    $#{$$prototype{'args'}} + 1, $nb ? "I" : "",
	    $$prototype{'return'}, $$prototype{'funcname'};
	}
	else {
	    $self->SUPER::function_start (@_);
	}
    }


    sub function_arg {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};

	if ($$prototype{'type'} !~ /^(varargs|stdarg)$/) {
	    my $argtype   = $params{'argtype'};
	    my $argname   = $params{'argname'};
	    my $argreg    = $params{'argreg'};
	    
	    print "	AROS_LCA($argtype, ($argname), " . uc $argreg . "), \\\n";
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
		print "	$$sfd{'basetype'}, $self->{BASE}, ";
	    }
	    else {
		print "	/* bt */, /* bn */, ";
	    }
	    
	    print $$prototype{'bias'} / 6;
	    print ", /* s */)\n";
	}
	else {
	    $self->SUPER::function_end (@_);
	}
    }
}
