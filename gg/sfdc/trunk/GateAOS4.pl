
### Class GateAOS4: Create a AmigaOS gatestub file ############################

BEGIN {
    package GateAOS4;
    use vars qw(@ISA);
    @ISA = qw( Gate );

    sub new {
	my $proto  = shift;
	my $class  = ref($proto) || $proto;
	my $self   = $class->SUPER::new( @_ );
	bless ($self, $class);
	return $self;
    }

    sub function {
	my $self     = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if ($prototype->{type} eq 'function' ||
	    $prototype->{type} eq 'varargs' ) {
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
    
    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	print "$prototype->{return}\n";
	print "$gateprefix$prototype->{funcname}(";
	print "struct $sfd->{BASENAME}IFace* _iface"
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

	print ",\n";
	print "	$prototype->{___args}[$argnum]";
    }
    
    sub function_end {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if ($self->{PROTO}) {
	    print ");\n";
	}
	else {
	    print ")\n";
	    print "{\n";
	    
	    if ($prototype->{type} ne 'varargs') {
		print "  return $libprefix$prototype->{funcname}(";

		if ($libarg eq 'first' && !$prototype->{nb}) {
		    print "($sfd->{basetype}) _iface->Data.LibBase";
		    print $prototype->{numargs} > 0 ? ", " : "";
		}

		print join (', ', @{$prototype->{___argnames}});

		if ($libarg eq 'last' && !$prototype->{nb}) {
		    print $prototype->{numargs} > 0 ? ", " : "";
		    print "($sfd->{basetype}) _iface->Data.LibBase";
		}
	    }
	    else {
		my $na;

		if ($prototype->{subtype} eq 'tagcall') {
		    $na = $prototype->{numargs} - 3;
		}
		elsif ($prototype->{subtype} eq 'printfcall') {
		    $na = $prototype->{numargs} - 2;
		}
		else {
		    # methodcall: first vararg is removed
		    $na = $prototype->{numargs} - 3;
		}
		
		print "  va_list _va;\n";
		print "  va_startlinear (_va, ";
		if ($na >= 0) {
		    print "$prototype->{___argnames}[$na]);\n";
		}
		else {
		    print "_iface);\n"
		}

		print "  return $prototype->{real_funcname}(";

		if ($libarg eq 'first' && !$prototype->{nb}) {
		    print "($sfd->{basetype}) _iface->Data.LibBase";
		    print $prototype->{numargs} > 0 ? ", " : "";
		}

		print "va_getlinearva (_va, " .
		    "$prototype->{argtypes}[$prototype->{numargs}-1])";
		
		for (my $i = 0; $i < $na; ++$i) {
		    print @{$prototype->{___argnames}}[$i];
		}

		if ($libarg eq 'last' && !$prototype->{nb}) {
		    print $prototype->{numargs} > 0 ? ", " : "";
		    print "($sfd->{basetype}) _iface->Data.LibBase";
		}
		
#		varargs = va_getlinearva(ap, struct TagItem *);
#		return	AllocAudioA(
#				    varargs,
#				    (struct AHIBase *) IAHI->Data.LibBase);
	    }
	
	    print ");\n";
	    print "}\n";
	}
    }
}
