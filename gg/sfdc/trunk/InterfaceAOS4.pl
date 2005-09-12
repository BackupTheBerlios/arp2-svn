
### Class Interface: Create a struct with function pointers ###################

BEGIN {
    package InterfaceAOS4;
    use vars qw(@ISA);
    @ISA = qw( Interface );

    sub new {
	my $proto  = shift;
	my $class  = ref($proto) || $proto;
	my $self   = $class->SUPER::new( @_ );
	bless ($self, $class);
	return $self;
    }

    # Helper functions
    
    sub define_interface_data {
	my $self     = shift;
	my $sfd      = $self->{SFD};

	print "#include <exec/interfaces.h>\n";
    }


    sub output_prelude {
	my $self     = shift;
	my $sfd      = $self->{SFD};

	print "\tstruct InterfaceData Data;\n";
	print "\n";
        print "\tULONG APICALL (*Obtain)(struct sfd->{BaseName}IFace *Self);\n";
	print "\tULONG APICALL (*Release)(struct sfd->{BaseName}IFace *Self);\n";
	print "\tvoid APICALL (*Expunge)(struct sfd->{BaseName}IFace *Self);\n";
	print "\tstruct Interface * APICALL (*Clone)(struct sfd->{BaseName}IFace *Self);\n";
    }
}
