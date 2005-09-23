
### Class Interface: Create a struct with function pointers ###################

BEGIN {
    package Interface;

    sub new {
	my $proto  = shift;
	my %params = @_;
	my $class  = ref($proto) || $proto;
	my $self   = {};
	$self->{SFD} = $params{'sfd'};
	$self->{BIAS} = -1;
	$self->{PADCNT} = 1;
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "/* Automatically generated function table! Do not edit! */\n";
	print "\n";
	print "#ifndef $sfd->{'BASENAME'}_INTERFACE_DEF_H\n";
	print "#define $sfd->{'BASENAME'}_INTERFACE_DEF_H\n";
	print "\n";

	foreach my $inc (@{$$sfd{'includes'}}) {
	    print "#include $inc\n";
	}

	foreach my $td (@{$$sfd{'typedefs'}}) {
	    print "typedef $td;\n";
	}

	print "\n";
	$self->define_interface_data();
	print "\n";

	print "struct $sfd->{BaseName}IFace\n";
	print "{\n";

	$self->output_prelude();
    }

    sub function {
	my $self      = shift;
	my $sfd       = $self->{SFD};
	my %params    = @_;
	my $prototype = $params{'prototype'};

	if ($self->{BIAS} == -1) {
	    $self->{BIAS} = $prototype->{bias} - 6;
	}

	while ($self->{BIAS} < ($prototype->{bias} - 6)) {
	    print "\tAPTR Pad$self->{PADCNT};\n";
	    $self->{BIAS} += 6;
	    ++$self->{PADCNT};
	}

	$self->{BIAS} = $prototype->{bias};

	print "\t$prototype->{return} APICALL ";
	print "(*$libprefix$prototype->{funcname})(struct $sfd->{BaseName}IFace* Self";
	if ($prototype->{numargs} != 0) {
	    print ", ";
	}
	print join (', ', @{$prototype->{args}});
	print ");\n";
    }
    
    sub footer {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "};\n";
	print "\n";
	print "#endif /* $sfd->{'BASENAME'}_INTERFACE_DEF_H */\n";
    }


    # Helper functions
    
    sub define_interface_data {
	my $self     = shift;
	my $sfd      = $self->{SFD};

	print "struct $sfd->{BaseName}InterfaceData {\n";
	print "\t$sfd->{basetype} LibBase;\n";
	print "\tAPTR Private;\n";
	print "};\n";
    }


    sub output_prelude {
	my $self     = shift;
	my $sfd      = $self->{SFD};

	print "\tstruct $sfd->{BaseName}InterfaceData Data;\n";
	print "\n";
    }
}
