
### Class GateAROS: Create an AROS gatestub file ##############################

BEGIN {
    package GateAROS;
    use vars qw(@ISA);
    @ISA = qw( Gate );

    sub new {
	my $proto  = shift;
	my $class  = ref($proto) || $proto;
	my $self   = $class->SUPER::new( @_ );
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	
	$self->SUPER::header (@_);

	print "#include <aros/libcall.h>\n";
	print "\n";
    }

    sub function_proto {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	Gate::print_libproto($sfd, $prototype);
	print ";\n\n";

	# AROS macros cannot handle function pointer arguments :-(

	for my $i (0 .. $prototype->{numargs} - 1) {
	    if ($prototype->{argtypes}[$i] =~ /\(\*\)/) {
		my $typedef  = $prototype->{argtypes}[$i];
		my $typename = "$sfd->{Basename}_$prototype->{funcname}_fp$i";

		$typedef =~ s/\(\*\)/(*_$typename)/;
		    
		print "typedef $typedef;\n";
	    }
	}
    }
    
    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};
	my $nb        = $sfd->{base} eq '' || $libarg eq 'none';

	printf "AROS_LH%d%s(", $prototype->{numargs}, $nb ? "I" : "";
	print "$prototype->{return}, $gateprefix$prototype->{funcname},\n";
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

	if ($argtype =~ /\(\*\)/) {
	    $argtype = "_$sfd->{Basename}_$prototype->{funcname}_fp$argnum";
	}
	
	print "	AROS_LHA($argtype, $argname, " . (uc $argreg) . "),\n";
    }
    
    sub function_end {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};
	my $nb        = $sfd->{base} eq '';

	my $bt = "/* bt */";
	my $bn = "/* bn */";

	if ($nb) {
	    for my $i (0 .. $#{$prototype->{regs}}) {
		if ($prototype->{regs}[$i] eq 'a6') {
		    $bt = $prototype->{argtypes}[$i];
		    $bn  =$prototype->{___argnames}[$i];
		    last;
		}
	    }
	}
	else {
	    $bt = $sfd->{basetype};
	    $bn = "_base";
	}

	printf "	$bt, $bn, %d, $sfd->{Basename})\n",
	$prototype->{bias} / 6;
	
	print "{\n";
	print "  return $libprefix$prototype->{funcname}(";

	if ($libarg eq 'first' && $sfd->{base} ne '') {
	    print "_base";
	    print $prototype->{numargs} > 0 ? ", " : "";
	}

	print join (', ', @{$prototype->{___argnames}});
	
	if ($libarg eq 'last' && $sfd->{base} ne '') {
	    print $prototype->{numargs} > 0 ? ", " : "";
	    print "_base";
	}
	
	print ");\n";
	print "}\n";
    }
}
