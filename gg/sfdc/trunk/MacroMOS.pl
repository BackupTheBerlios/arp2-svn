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

### Class MacroMOS: Implements MorphOS-only features for macro files ###########

BEGIN {
    package MacroMOS;
    use vars qw(@ISA);
    @ISA = qw (Macro);

    sub new {
	my $proto  = shift;
	my $class  = ref($proto) || $proto;
	my $self   = $class->SUPER::new( @_ );
	bless ($self, $class);
	return $self;
    }

    sub function_end {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	
	if ($$prototype{'type'} !~ /^(varargs|stdarg)$/) {

	    if( $$sfd{'base'} ne '') {
		print "\\\n	, $self->{BASE}, ";
	    }
	    else {
		print ", ";
	    }

	    print "IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)\n";
	}
	else {
	    print "})\n";
	}
    }
}
