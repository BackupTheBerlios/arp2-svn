
### Class SASPragmas: Create a SAS/C pragmas file #############################

BEGIN {
    package SASPragmas;

    sub new {
	my $proto  = shift;
	my %params = @_;
	my $class  = ref($proto) || $proto;
	my $self   = {};
	$self->{SFD} = $params{'sfd'};
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	my $sfd  = $self->{SFD};

	my $id = $$sfd{'id'};
	my $v  = $id;
	my $d  = $id;

	$v =~ s/^\$[I]d: .*? ([0-9.]+).*/$1/;
	$d =~ s,^\$[I]d: .*? [0-9.]+ (\d{4})/(\d{2})/(\d{2}).*,($3.$2.$1),;

	print "/* Automatically generated header! Do not edit! */\n";
	print "#ifndef PRAGMAS_$sfd->{BASENAME}_PRAGMAS_H\n";
	print "#define PRAGMAS_$sfd->{BASENAME}_PRAGMAS_H\n";
	print "\n";
	print "/*\n";
	print "**	\$VER: $$sfd{'basename'}_pragmas.h $v $d\n";
	print "**\n";
	print "**	Direct ROM interface (pragma) definitions.\n";
	print "**\n";
	print "**	$$sfd{'copyright'}\n";
	print "**	    All Rights Reserved\n";
	print "*/\n";
	print "\n";
    }

    sub function {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	my $regs = '';
	
	foreach my $reg (@{$prototype->{regs}}) {
	    my $num;
	    
	    if ($reg =~ /^d[0-7]$/) {
		($num) = $reg =~ /^d(.)/;
	    }
	    elsif ($reg =~ /^a[0-9]$/) {
		($num) = $reg =~ /^a(.)/;
		$num += 8;
	    }
	    else {
		die;
	    }

	    $regs = sprintf "%x$regs", $num;
	}

	$regs .= '0'; #Result in d0
	$regs .= $prototype->{numregs};
	
	if ($prototype->{type} eq 'function' ||
	    $prototype->{type} eq 'alias' ) {

	    # Always use libcall, since access to 4 is very expensive
	    
	    print "#pragma libcall $sfd->{base} $prototype->{funcname} ";
	    printf "%x $regs\n", $prototype->{bias};
	}
	else {
	    print "#ifdef __SASC_60\n";
	    print "#pragma tagcall $sfd->{base} $prototype->{funcname} ";
	    printf "%x $regs\n", $prototype->{bias};
	    print "#endif\n";
	}
    }
    
    sub footer {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "\n";
	print "#endif /* PRAGMAS_$sfd->{BASENAME}_PRAGMAS_H */\n";
    }
}
