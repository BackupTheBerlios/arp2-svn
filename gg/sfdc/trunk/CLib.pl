#
#     SFDCompile - Compile SFD files into someting useful
#     Copyright (C) 2003 Martin Blom <martin@blom.org>
#     
#     This program is free software; you can redistribute it and/or
#     modify it under the terms of the GNU General Public License
#     as published by the Free Software Foundation; either version 2
#     of the License, or (at your option) any later version.
#     
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#     
#     You should have received a copy of the GNU General Public License
#     along with this program; if not, write to the Free Software
#     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

use strict;

### Class CLib: Create a clib file #############################################

BEGIN {
    package CLib;

    sub new {
	my $proto  = shift;
	my %params = @_;
	my $class  = ref($proto) || $proto;
	my $self   = {};
	$self->{SFD}     = $params{'sfd'};
	$self->{VERSION} = 1;
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
	print "\n";
	print "#ifndef CLIB_$$sfd{'BASENAME'}_PROTOS_H\n";
	print "#define CLIB_$$sfd{'BASENAME'}_PROTOS_H\n";
	print "\n";
	print "/*\n";
	print "**	\$VER: $$sfd{'basename'}_protos.h $v $d\n";
	print "**\n";
	print "**	C prototypes. For use with 32 bit integers only.\n";
	print "**\n";
	print "**	$$sfd{'copyright'}\n";
	print "**	    All Rights Reserved\n";
	print "*/\n";
	print "\n";

	foreach my $inc (@{$$sfd{'includes'}}) {
	    print "#include $inc\n";
	}

	print "\n";
	print "#ifdef __cplusplus\n";
	print "extern \"C\" {\n";
	print "#endif /* __cplusplus */\n";
	print "\n";

	$self->{VERSION} = 1;
    }

    sub function {
	my $self     = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};
	
	if ($self->{VERSION} != $$prototype{'version'}) {
	    $self->{VERSION} = $$prototype{'version'};

	    print "\n";
	    print "/*--- functions in V$self->{VERSION} or higher ---*/\n";
	}
	
	if ($$prototype{'comment'} ne '') {
	    my $comment = $$prototype{'comment'};

	    $comment =~ s,^(\s?)(.*),/*$1$2$1*/,m;
		
	    print "\n";
	    print "$comment\n";
	}
	
	my $args = join (', ',@{$$prototype{'args'}});

	if ($args eq '') {
	    $args = "void";
	}
	
	print "$$prototype{'return'} $$prototype{'funcname'}($args);\n";
    }

    sub footer {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "\n";
	print "#ifdef __cplusplus\n";
	print "}\n";
	print "#endif /* __cplusplus */\n";
	print "\n";
	print "#endif /* CLIB_$$sfd{'BASENAME'}_PROTOS_H */\n";
    }
}
