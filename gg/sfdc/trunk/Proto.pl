#!/usr/bin/perl -w                                      -- # -*- perl -*-
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

### Class Proto: Create a proto file ###########################################

BEGIN {
    package Proto;

    sub new {
	my $proto    = shift;
	my %params   = @_;
	my $class    = ref($proto) || $proto;
	my $self     = {};
	$self->{SFD} = $params{'sfd'};
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	my $sfd  = $self->{SFD};

	my $base     = $$sfd{'base'};
	my $basename = $$sfd{'basename'};
	my $BASENAME = $$sfd{'BASENAME'};
	my $basetype = $$sfd{'basetype'};

	print "/* Automatically generated header! Do not edit! */\n";
	print "\n";
	print "#ifndef PROTO_${BASENAME}_H\n";
	print "#define PROTO_${BASENAME}_H\n";
	print "\n";
	print "#include <clib/${basename}_protos.h>\n";
	print "\n";
	print "#ifndef _NO_INLINE\n";
	print "# ifdef __GNUC__\n";
	print "#  ifdef __amigaos4__\n";
	print "#   ifdef __USE_INLINE__\n";
	print "#    include <inline4/${basename}.h>\n";
	print "#   endif /* __USE_INLINE__ */\n";
	print "#  else /* !__amigaos4__ */\n";
	print "#   include <inline/${basename}.h>\n";
	print "#  endif /* __amigaos4__ */\n";
	print "# endif /* __GNUC__ */\n";
	print "#endif /* _NO_INLINE */\n";
	print "\n";

	if ($base ne '') {
	    print "#ifdef __amigaos4__\n";
	    print "# include <interfaces/${basename}.h>\n";
	    print "# ifndef __NOGLOBALIFACE__\n";
	    print "   extern struct ${base}IFace *I${base};\n";
	    print "# endif /* __NOGLOBALIFACE__*/\n";  
	    print "#else /* !__amigaos4__ */\n";
	    print "# ifndef __NOLIBBASE__\n";
	    print "   extern ${basetype}\n";
	    print "#  ifdef __CONSTLIBBASEDECL__\n";
	    print "    __CONSTLIBBASEDECL__\n";
	    print "#  endif /* __CONSTLIBBASEDECL__ */\n";
	    print "   ${base};\n";
	    print "# endif /* !__NOLIBBASE__ */\n";
	    print "\n";
	}
    }

    sub function {
	# Nothing to do here ...
    }

    sub footer {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "#endif /* !PROTO_$$sfd{'BASENAME'}_H */\n";
    }
}
