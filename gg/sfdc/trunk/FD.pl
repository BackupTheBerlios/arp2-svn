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

### Class FD: Create an old-style FD file ######################################

BEGIN {
    package FD;

    sub new {
	my $proto  = shift;
	my %params = @_;
	my $class  = ref($proto) || $proto;
	my $self   = {};
	$self->{SFD}     = $params{'sfd'};
	$self->{BIAS}    = -1;
	$self->{PRIVATE} = -1;
	$self->{VERSION} = 1;
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "* \"$$sfd{'libname'}\"\n";
	print "* Automatically generated FD! Do not edit!\n";
	print "##base _$$sfd{'base'}\n";
	$self->{BIAS}    = -1;
	$self->{PRIVATE} = -1;
	$self->{VERSION} = 1;
    }

    sub function {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if ($$prototype{'type'} eq 'function') {
	    if ($self->{BIAS} != $$prototype{'bias'}) {
		$self->{BIAS} = $$prototype{'bias'};
		print "##bias $self->{BIAS}\n";
	    }

	    if ($self->{PRIVATE} != $$prototype{'private'}) {
		$self->{PRIVATE} = $$prototype{'private'};
		print $self->{PRIVATE} == 1 ? "##private\n" : "##public\n";
	    }

	    if ($self->{VERSION} != $$prototype{'version'}) {
		$self->{VERSION} = $$prototype{'version'};

		print "*--- functions in V$self->{VERSION} or higher ---\n";
	    }

	    if ($$prototype{'comment'} ne '') {
		my $comment = $$prototype{'comment'};

		$comment =~ s/^/\*/m;
		
		print "$comment\n";
	    }
	    
	    print "$$prototype{'funcname'}(";
	    print join (',', @{$$prototype{'argnames'}});
	    print ")(";
	    print join (',', @{$$prototype{'regs'}});
	    print ")\n";
	
	    $self->{BIAS} += 6;
	}
    }
    
    sub footer {
	my $self = shift;

	print "##end\n";
    }
}
