
### Class Stub: Create a generic stub file #####################################

BEGIN {
    package Stub;

    sub new {
	my $proto  = shift;
	my %params = @_;
	my $class  = ref($proto) || $proto;
	my $self   = {};
	$self->{SFD}     = $params{'sfd'};
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "/* Automatically generated stubs! Do not edit! */\n";
	print "\n";

	foreach my $inc (@{$$sfd{'includes'}}) {
	    print "#include $inc\n";
	}

	print "\n";
	print "#ifdef __cplusplus\n";
	print "extern \"C\" {\n";
	print "#endif /* __cplusplus */\n";
	print "\n";

	if ($$sfd{'base'} ne '') { 
	    print "#ifndef BASE_EXT_DECL\n";
	    print "#define BASE_EXT_DECL\n";
	    print "#define BASE_EXT_DECL0 extern $$sfd{'basetype'} " .
		"$$sfd{'base'};\n";
	    print "#endif /* !BASE_EXT_DECL */\n";
	    print "#ifndef BASE_PAR_DECL\n";
	    print "#define BASE_PAR_DECL\n";
	    print "#define BASE_PAR_DECL0 void\n";
	    print "#endif /* !BASE_PAR_DECL */\n";
	    print "#ifndef BASE_NAME\n";
	    print "#define BASE_NAME $$sfd{'base'}\n";
	    print "#endif /* !BASE_NAME */\n";
	    print "\n";
	    print "BASE_EXT_DECL0\n";
	    print "\n";
	}

    }

    sub function {
	my $self     = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};
	
	my $args = join (', ',@{$$prototype{'args'}});

	if ($args eq '') {
	    $args = "void";
	}
	
	print "$$prototype{'return'}\n";
	print "$$prototype{'funcname'}(";
	if ($$sfd{'base'} ne '') {
	    if ($$prototype{'numargs'} == 0) {
		print "BASE_PAR_DECL0";
	    }
	    else {
		print "BASE_PAR_DECL, ";
	    }
	}
	print join (', ', @{$$prototype{'___args'}});
	print ")\n";
	print "{\n";
	print "\n";
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
	
#	$self->function_define (prototype => $prototype);
#	$self->function_start (prototype => $prototype);
#	for my $i (0 .. $$prototype{'numargs'} - 1 ) {
#	    $self->function_arg (prototype => $prototype,
#				 argtype   => $$prototype{'argtypes'}[$i],
#				 argname   => $$prototype{'___argnames'}[$i],
#				 argreg    => $$prototype{'regs'}[$i],
#				 argnum    => $i );
#	}
#	$self->function_end (prototype => $prototype);

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

	print "\n";
	print "#undef BASE_EXT_DECL\n";
	print "#undef BASE_EXT_DECL0\n";
	print "#undef BASE_PAR_DECL\n";
	print "#undef BASE_PAR_DECL0\n";
	print "#undef BASE_NAME\n";
	print "\n";
	print "#ifdef __cplusplus\n";
	print "}\n";
	print "#endif /* __cplusplus */\n";
    }
}
