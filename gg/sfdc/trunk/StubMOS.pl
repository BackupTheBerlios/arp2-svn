
### Class StubMOS: Create a MorphOS stub file #################################

BEGIN {
    package StubMOS;
    use vars qw(@ISA);
    @ISA = qw( Stub );

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

	print "\n";
	print "#include <emul/emulregs.h>\n";
	print "#include <stdarg.h>\n";
	print "\n";
    }
    
    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};
	my $nb        = $sfd->{base} eq '';

	if ($prototype->{type} !~ /^(varargs)|(stdarg)$/) {
	    print "\n";
	    print "{\n";

	    if (!$nb) {
		print "  BASE_EXT_DECL\n";
	    }
	}
	elsif ($prototype->{type} eq 'varargs') {
	    my $na = $prototype->{numargs};
	    print "\n";
	    print "{\n";
	    print "  va_list _va;\n";
	    print "  va_start (_va, $prototype->{___argnames}[$na-2]);\n";
	    print "  return $$prototype{'real_funcname'}(BASE_PAR_NAME ";
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

	if ($$prototype{'type'} !~ /^(stdarg)|(varargs)$/) {
	    print "  REG_" . (uc $argreg) . " = (ULONG) $argname;\n";
	}
	elsif ($prototype->{type} eq 'varargs' &&
	       $argnum == $prototype->{numargs} - 1) {
	    my $vartype  = $$prototype{'argtypes'}[$$prototype{'numargs'} - 1];
	    my $argnm = $$prototype{'___argnames'}[$$prototype{'numargs'} - 2];
	    print ", ($vartype) _va->overflow_arg_area";
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

	
	if ($$prototype{'type'} !~ /^(varargs)|(stdarg)$/) {
	    my $nr = $prototype->{return} =~ /^(VOID|void)$/;
	    my $nb = $sfd->{base} eq '';

	    if (!$nb) {
		print "  REG_A6 = (ULONG) BASE_NAME;\n";
	    }

	    print "  ";
	    
	    if (!$nr) {
		print "return ($prototype->{return}) ";
	    }

	    print "(*MyEmulHandle->EmulCallDirectOS)(-$prototype->{bias});\n";
	    print "}\n";
	}
	else {
	    $self->SUPER::function_end (@_);
	}
    }
}
