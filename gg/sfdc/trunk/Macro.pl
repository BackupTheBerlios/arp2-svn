
### Class Macro: Create a macro file ###########################################

# Macros are a bit different than those generated by fd2inline.
#
# Tag lists ("stdarg") are always initialized with the first tag value
# followed by __VA_ARGS__. This generates a compile-time error if no tags
# are supplied (TAG_DONE is the minimal tag list).

BEGIN {
    package Macro;

    sub new {
	my $proto  = shift;
	my %params = @_;
	my $class  = ref($proto) || $proto;
	my $self   = {};
	$self->{SFD}  = $params{'sfd'};
	$self->{BASE} = "${$self->{SFD}}{'BASENAME'}_BASE_NAME";
	$self->{BASE} =~ s/^([0-9])/_$1/;
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
	print "#ifndef __INLINE_MACROS_H\n";
	print "#include <inline/macros.h>\n";
	print "#endif /* !__INLINE_MACROS_H */\n";
	print "\n";

	if ($$sfd{'base'} ne '') {
	    print "#ifndef $self->{BASE}\n";
	    print "#define $self->{BASE} $$sfd{'base'}\n";
	    print "#endif /* !$self->{BASE} */\n";
	    print "\n";
	}
    }

    sub function {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if ($$prototype{'type'} eq 'stdarg') {
	    print "#ifndef NO_INLINE_STDARG\n";
	}
	elsif ($$prototype{'type'} eq 'varargs') {
	    print "#ifndef NO_INLINE_VARARGS\n";
	}
	
	$self->function_define (prototype => $prototype);
	$self->function_start (prototype => $prototype);
	for my $i (0 .. $#{$$prototype{'args'}}) {
	    $self->function_arg (prototype => $prototype,
				 argtype   => $$prototype{'argtypes'}[$i],
				 argname   => $$prototype{'___argnames'}[$i],
				 argreg    => $$prototype{'regs'}[$i],
				 argnum    => $i );
	}
	$self->function_end (prototype => $prototype);

	if ($$prototype{'type'} eq 'stdarg') {
	    print "#endif /* !NO_INLINE_STDARG */\n";
	}
	elsif ($$prototype{'type'} eq 'varargs') {
	    print "#endif /* !NO_INLINE_VARARGS */\n";
	}

	print "\n";
    }

    sub footer {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "#endif /* !_INLINE_$$sfd{'BASENAME'}_H */\n";
    }


    # Helper functions
    
    sub function_define {
	my $self     = shift;
	my %params   = @_;
	my $prototype = $params{'prototype'};
	my $sfd      = $self->{SFD};

	print "#define $$prototype{'funcname'}(";
	print join (', ', @{$$prototype{'___argnames'}});
	print ") \\\n";
    }

    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};
	my $nr        = $$prototype{'return'} =~ /^(VOID|void)$/;
	my $nb        = $$sfd{'base'} eq '';

	if ($$prototype{'type'} eq 'stdarg') {
	    my $first_stdargnum = $#{$$prototype{'argnames'}} - 1;
	    my $first_stdarg = $$prototype{'argnames'}[$first_stdargnum];
	    
	    print "	({ULONG _tags[] = { $first_stdarg, __VA_ARGS__ }; ";
	    print "$$prototype{'real_funcname'}(";	    
	}
	elsif ($$prototype{'type'} eq 'varargs' ) {
	    print "	({ULONG _args[] = { __VA_ARGS__ }; ";

	    print "$$prototype{'real_funcname'}(";
	}
	else {
	    printf "	LP%d%s%s(0x%x, ", $#{$$prototype{'___args'}} + 1,
	    $nr ? "NR" : "", $nb ? "NB" : "", $$prototype{'bias'};

	    if (!$nr) {
		print "$$prototype{'return'}, ";
	    }

	    print "$$prototype{'funcname'}, ";
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

	if ($$prototype{'type'} eq 'varargs') {
	    if ($argname eq '...') {
		print "($argtype) _args);";
	    }
	    else {
		print "($argname), ";
	    }
	}
	elsif ($$prototype{'type'} eq 'stdarg' ) {
	    my $first_stdargnum = $#{$$prototype{'___argnames'}} - 1;

	    # Skip the first stdarg completely
	    if( $argnum != $first_stdargnum ) {
		if ($argname eq '...') {
		    print "($argtype) _tags);";
		}
		else {
		    print "($argname), ";
		}
	    }
	}
	else {
	    print "$argtype, $argname, $argreg, ";
	}
    }
    
    sub function_end {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	
	if ($$prototype{'type'} !~ /^(varargs|stdarg)$/ &&
	    $$sfd{'base'} ne '') {
	    print "\\\n	, $self->{BASE})\n";
	}
	else {
	    print "})\n";
	}
    }
}
