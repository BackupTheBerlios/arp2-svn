
### Class Stub68k: Create a 68k stub file #####################################

BEGIN {
    package Stub68k;
    use vars qw(@ISA);
    @ISA = qw( Stub );

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

	if ($prototype->{type} !~ /^(varargs)|(stdarg)$/) {
	    print "\n";
	    print "{\n";

	    if (!$prototype->{nb}) {
		print "  BASE_EXT_DECL\n";
	    }
	    if (!$prototype->{nr}) {
		print "  register $prototype->{return} _res __asm(\"d0\");\n";
	    }
	    if (!$prototype->{nb}) {
		print "  register $sfd->{basetype} _base __asm(\"a6\") " .
		    "= BASE_NAME;\n";
	    }
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
	    print "  register $prototype->{args}[$argnum] __asm(\"$argreg\") " .
		"= $argname;\n";
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
	    print "  __asm volatile (\"jsr a6@(-$prototype->{bias}:W)\"\n";
	    print "  : " .
		($prototype->{nr} ? "/* No output */" : '"=r" (_res)') . "\n";
	    print "  : ";
	    if (!$prototype->{nb}) {
		print '"r" (_base)';
	    }

	    for my $i (0 .. $prototype->{numargs} - 1) {
		if ($i != 0 || !$prototype->{nb}) {
		    print ", ";
		}
		
		print '"r" (' . $prototype->{argnames}[$i] . ')';
	    }

	    print "\n";
	    print '  : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");';
	    print "\n";
	    
	    if (!$prototype->{nr}) {
		print "  return _res;\n";
	    }

	    print "}\n";
	}
	else {
	    $self->SUPER::function_end (@_);
	}
    }
}
