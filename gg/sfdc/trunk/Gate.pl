
### Class Gate: Create a generic gate file ####################################

BEGIN {
    package Gate;

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

	print "/* Automatically generated gatestubs! Do not edit! */\n";
	print "\n";

	foreach my $inc (@{$$sfd{'includes'}}) {
	    print "#include $inc\n";
	}

	print "\n";
	print "#ifdef __cplusplus\n";
	print "extern \"C\" {\n";
	print "#endif /* __cplusplus */\n";
	print "\n";
    }

    sub function {
	my $self     = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if ($prototype->{type} eq 'function') {
	    $self->function_proto (prototype => $prototype);
	    $self->function_start (prototype => $prototype);
	    for my $i (0 .. $$prototype{'numargs'} - 1 ) {
		$self->function_arg (prototype => $prototype,
				     argtype   => $$prototype{'argtypes'}[$i],
				     argname   => $$prototype{'___argnames'}[$i],
				     argreg    => $$prototype{'regs'}[$i],
				     argnum    => $i );
	    }
	    $self->function_end (prototype => $prototype);
	    
	    print "\n";
	}
    }

    sub footer {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "\n";
	print "#ifdef __cplusplus\n";
	print "}\n";
	print "#endif /* __cplusplus */\n";
    }


    # Helper functions
    
    sub function_proto {
	my $self     = shift;
	my %params   = @_;
	my $prototype = $params{'prototype'};
	my $sfd      = $self->{SFD};

	print_libproto($sfd, $prototype);
	print ";\n\n";	
    }

    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};
	
	print_gateproto ($sfd, $prototype);
	print "\n";
	print "{\n";
	print "  return $libprefix$prototype->{funcname}(";

	if ($libarg eq 'first' && $sfd->{base} ne '') {
	    print "_base";
	    print $prototype->{numargs} > 0 ? ", " : "";
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

	print $argnum > 0 ? ", " : "";
	print $argname;
    }

    sub function_end {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};
	
	if ($libarg eq 'last' && $sfd->{base} ne '') {
	    print $prototype->{numargs} > 0 ? ", " : "";
	    print "_base";
	}
	    
	print ");\n";
	print "}\n";
    }


    sub print_gateproto {
	my $sfd       = shift;
	my $prototype = shift;
	
	print "$prototype->{return}\n";
	print "$gateprefix$prototype->{funcname}(";

	if ($libarg eq 'first' && $sfd->{base} ne '') {
	    print "$sfd->{basetype} _base";
	    print $prototype->{numargs} > 0 ? ", " : "";
	}
	
	print join (', ', @{$prototype->{___args}});

	if ($libarg eq 'last' && $sfd->{base} ne '') {
	    print $prototype->{numargs} > 0 ? ", " : "";
	    print "$sfd->{basetype} _base";
	}

	if ($libarg eq 'none' && $prototype->{numargs} == 0) {
	    print "void";
	}

	print ")";
    }
    sub print_libproto {
	my $sfd       = shift;
	my $prototype = shift;
	
	print "$prototype->{return}\n";
	print "$libprefix$prototype->{funcname}(";

	if ($libarg eq 'first' && $sfd->{base} ne '') {
	    print "$sfd->{basetype} _base";
	    print $prototype->{numargs} > 0 ? ", " : "";
	}
	    
	print join (', ', @{$prototype->{___args}});

	if ($libarg eq 'last' && $sfd->{base} ne '') {
	    print $prototype->{numargs} > 0 ? ", " : "";
	    print "$sfd->{basetype} _base";
	}

	if ($libarg eq 'none' && $prototype->{numargs} == 0) {
	    print "void";
	}
	
	print ")";
    }
}
