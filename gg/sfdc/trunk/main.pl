#!/usr/bin/perl -w                                      -- # -*- perl -*-
#
#     sfdc - Compile SFD files into someting useful
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

use IO::Handle;
use Getopt::Long;
use Pod::Usage;

sub parse_sfd ( $ );
sub parse_proto ( $$ );
sub open_output ( $$ );
sub will_close_output ( $$ );
sub close_output ();

my %targets = (
	      '(\w)+(-.*)?-aros'              =>
	       { target => 'aros',
		 macros => 'MacroAROS',
		 stubs  => 'StubAROS' },
	       
	      'i.86be(-pc)?-amithlon'         =>
	       { target => 'amithlon',
		 macros => 'MacroLP',
		 stubs  => 'StubAmithlon' },
	       
	      'm68k(-unknown)?-amigaos'       =>
	       { target => 'amigaos',
		 macros => 'Macro68k',
		 stubs  => 'Stub68k' },
	       
	      'p(ower)?pc(-unknown)?-morphos' =>
	       { target => 'morphos',
		 macros => 'MacroMOS',
		 stubs  => 'StubMOS' },
	      );

my $classes;

###############################################################################
### Main program ##############################################################
###############################################################################

Getopt::Long::Configure ("bundling");

my $help    = '0';
my $man     = '0';
my $mode    = 'dump';
my $output  = '-';
my $quiet   = '0';
my $target  = 'm68k-unknown-amigaos';
my $version = '0';

GetOptions ('help|h'     => \$help,
	    'man'        => \$man,
	    'mode=s'     => \$mode,
	    'output|o=s' => \$output,
	    'quiet|q'    => \$quiet,
	    'target=s'   => \$target,
	    'version|v'  => \$version) or exit 10;

if ($version) {
    print STDERR "sfdc 1.0 (16.07.2003)\n";
    print STDERR "Copyright (C) 2003 Martin Blom <martin\@blom.org>\n";
    print STDERR "This is free software; " .
	"see the source for copying conditions.\n";
    exit 0;
}

if ($help) {
    pod2usage (-verbose => 1,
	       -exitval => 0,
	       -output => \*STDOUT);
}

if ($man) {
    pod2usage (-verbose => 3,
	       -exitval => 0);
    exit 0;
}

if ($#ARGV < 0) {
    pod2usage (-message => "No SFD file specified.",
	       -verbose => 0,
	       -exitval => 10);
}

$mode = lc $mode;

if (!($mode =~ /^(clib|dump|fd|macros|proto|stubs)$/)) {
    pod2usage (-message => "Unknown mode specified. Use --help for a list.",
	       -verbose => 0,
	       -exitval => 10);
}

check_target: {
    foreach my $target_regex (keys %targets) {
	if ($target =~ /^$target_regex$/) {
	    $classes = $targets{$target_regex};
	    last check_target;
	}
    }

    pod2usage (-message => "Unknown target specified. Use --help for a list.",
	       -verbose => 0,
	       -exitval => 10);
}

# Save old STDOUT

open( OLDOUT, ">&STDOUT" );

for my $i ( 0 .. $#ARGV ) {
    my $sfd = parse_sfd ($ARGV[$i]);
    my $num = $#{$$sfd{'prototypes'}};

    my $obj;

    for ($mode) {
	/^clib$/ && do {
	    $obj = CLib->new( sfd => $sfd );
	    last;
	};

	/^fd$/ && do {
	    $obj = FD->new( sfd => $sfd );
	    last;
	};
    
	/^dump$/ && do {
	    $obj = Dump->new( sfd => $sfd );
	    last;
	};
    
	/^macros$/ && do {
	    $obj = $$classes{'macros'}->new( sfd => $sfd );

	    # By tradition, the functions in the macro files are sorted
	    @{$$sfd{'prototypes'}} = sort {
		$$a{'funcname'} cmp $$b{'funcname'}
	    } @{$$sfd{'prototypes'}};
	    last;
	};

	/^proto$/ && do {
	    $obj = Proto->new( sfd => $sfd );
	    last;
	};

	/^stubs$/ && do {
	    $obj = $$classes{'stubs'}->new( sfd => $sfd );

	    # By tradition, the functions in the stub files are sorted
	    @{$$sfd{'prototypes'}} = sort {
		$$a{'funcname'} cmp $$b{'funcname'}
	    } @{$$sfd{'prototypes'}};
	    last;
	};
	
	die "Unknown mode specified: " . $mode;
    }


    for my $j ( 0 .. $num + 1) {
	my $prototype = $$sfd{'prototypes'}[$j];
	my $funcname  = $$prototype{'funcname'};
	
	if (!defined ($funcname) || will_close_output ($sfd, $funcname) != 0) {
	    $obj->footer ();
	}

	if ($j > $num) {
	    last;
	}
	
	if (open_output ($sfd, $funcname) != 0) {
	    $obj->header ();
	}

	$obj->function (prototype => $prototype);
    }

    close_output ();
}

if (!$quiet) {
    print STDERR "All done.\n";
}

open (STDOUT, ">&OLDOUT");
close (OLDOUT);

exit 0;






###############################################################################
### Subroutines ###############################################################
###############################################################################


### parse_sfd: Parse a SFD file hand return a hash record #####################

sub parse_sfd ( $ ) {
    my $file = shift;
    local *SFD;

    my $type    = 'function';
    my $private = 0;
    my $bias    = 0;
    my $version = 1;
    my $comment = '';

    my $result = {
	copyright  => 'Copyright © 2001 Amiga, Inc.',
	id         => '',
	libname    => '',
	base       => '',
	basetype   => '',
	includes   => (),
	prototypes => (),
	basename   => '',
	BASENAME   => '',
	Basename   => ''
    };

    # Why do I need this????
    $$result{'includes'} = ();

    my $proto_line = '';
    my %proto;

    if (!$quiet) {
	( my $fn = $file ) =~ s,.*[/\\](.*),$1,;
	print STDERR "Processing SFD file '$fn'.\n";
	STDERR->flush();
    }
    
    unless (open (SFD, "<" . $file)) {
	print STDERR "Unable to open file '$file'.\n";
	die;
    };

    my $line_no = 0;

  LINE:
    while (my $line = <SFD>) {

	++$line_no;
	
	for ($line) {
	    /==copyright\s/ && do {
		( $$result{'copyright'} = $_ ) =~ s/==copyright\s+(.*)\s*/$1/;
		last;
	    };

	    /==id\s+/ && do {
		( $$result{'id'} = $_ ) =~ s/==id\s+(.*)\s*/$1/;
		last;
	    };

	    /==libname\s+/ && do {
		( $$result{'libname'} = $_ ) =~ s/==libname\s+(.*)\s*/$1/;
		last;
	    };

	    /==base\s+/ && do {
		( $$result{'base'} = $_ ) =~ s/==base\s+_?(.*)\s*/$1/;
		last;
	    };

	    /==basetype\s+/ && do {
		( $$result{'basetype'} = $_ ) =~ s/==basetype\s+(.*)\s*/$1/;
		last;
	    };

	    /==include\s+/ && do {
		( my $inc = $_ ) =~ s/==include\s+(.*)\s*/$1/;

		push @{$$result{'includes'}}, $inc;
		last;
	    };
	    
	    /==bias\s+/ && do {
		( $bias = $_ ) =~ s/==bias\s+(.*)\s*/$1/;
		last;
	    };

	    /==reserve\s+/ && do {
		( my $reserve = $_ ) =~ s/==reserve\s+(.*)\s*/$1/;

		$bias += 6 * $reserve;
		last;
	    };

	    /==alias\s*$/ && do {
		$type = 'alias';
		# Move back again
		$bias -= 6;
		last;
	    };

	    /==varargs\s*$/ && do {
		$type = 'varargs';
		# Move back again
		$bias -= 6;
		last;
	    };
	    
	    /==private\s*$/ && do {
		$private = 1;
		last;
	    };

	    /==public\s*$/ && do {
		$private = 0;
		last;
	    };

	    /==version\s+/ && do {
		( $version = $_ ) =~ s/==version\s+(.*)\s*/$1/;
		last;
	    };
	    
	    /==end\s*$/ && do {
		last LINE;
	    };
	    
	    /^\*/ && do {
		( my $cmt = $_ ) =~ s/^\*(.*)\s*/$1/;

		$comment .= ($comment eq '' ? "" : "\n" ) . $cmt;
		last;
	    };
	    
	    /^[^=*\n]/ && do {
		# Strip whitespaces and append
		$line =~ s/\s*(.*)\s*/$1/;
		$proto_line .= $line . " ";
		last;
	    };

	    /^\s*$/ && do {
		# Skip blank lines
		last;
	    };

	    # If we get here, we found a line we don't understand
	    print STDERR "Unable to parse line $line_no in SFD file" .
		" '$file'. The line looks like this:\n" . $line ;
	    die;
	};
	
	if ( $proto_line =~
	     /.*[A-Za-z0-9_]+\(.*\).*\(([\saAdD][0-7]-?,?)*\)\s*$/ ) {
	    push @{$$result{'prototypes'}}, {
		type    => $type,
		value   => $proto_line,
		line    => $line_no,
		private => $private,
		bias    => $bias,
		version => $version,
		comment => $comment
	    };

	    $type       = 'function';
	    $comment    = '';
	    $proto_line = '';
	    $bias += 6;
	}
    }

    if( $proto_line ne '' ) {
	# If $proto_line isn't empty, we couldn't parse it
	die "Unhanled proto '" . $proto_line . "'\n";
    }

    close (SFD);

    # Now parse the prototypes
    my $real_funcname  = '';
    my $real_prototype = {};
    my $varargs_type   = '';

    for my $i ( 0 .. $#{$$result{'prototypes'}} ) {
	my $prototype = $$result{'prototypes'}[$i];

	if ($$prototype{'type'} ne 'function') {
	    $$prototype{'real_funcname'}  = $real_funcname;
	    $$prototype{'real_prototype'} = $real_prototype;
	}
	
	parse_proto ($prototype, $varargs_type);

	if ($$prototype{'type'} ne 'varargs' &&
	    $$prototype{'type'} ne 'stdarg') {
	    $varargs_type = $$prototype{'argtypes'}[$#{$$prototype{'argtypes'}}];
	}

	if ($$prototype{'type'} eq 'function') {
	    $real_funcname  = $$prototype{'funcname'};
	    $real_prototype = $prototype;
	}
    };

    # Create some other variables

    ( $$result{'basename'} = $$result{'libname'} ) =~ s/(.*)\.\w+/$1/;

    if ($$result{'basename'} eq '') {
	( $$result{'basename'} = $file ) =~ s:.*/(\w+?)_lib\.sfd:$1:;
    }

    if ($$result{'basename'} eq '') {
	print STDERR "Unable to find or guess base name.\n";
	die;
    }
    
    $$result{'basename'} = lc $$result{'basename'};
    $$result{'BASENAME'} = uc $$result{'basename'};
    $$result{'Basename'} = ucfirst $$result{'basename'};

    return $result;
}


### parse_proto: Parse a single function prototype  ###########################

sub parse_proto ( $$ ) {
    my $prototype      = shift;
    my $varargs_type   = shift;
    
    my $return;
    my $struct;     # Just a dummy
    my $name;
    my $arguments;
    my $registers;

    if (!(($return,$struct,$name,$arguments,$registers) =
	  ( $$prototype{'value'} =~
	    /^((struct\s+)?\w+\s*?\**)\s*(\w+)\s*\((.*)\)\s*\((.*)\).*/ ))) {
	print STDERR "Unable to parse prototype on line $$prototype{'line'}.\n";
	die;
    }

    # Nuke whitespaces from the register specification
    $registers =~ s/\s//;

    $$prototype{'return'}   = $return;
    $$prototype{'funcname'} = $name;

    $$prototype{'numargs'}  = 0;
    $$prototype{'numregs'}  = 0;
    
    @{$$prototype{'regs'}}  = split(/,/,lc $registers);  # Make regs lower case
    
    @{$$prototype{'args'}}        = ();
    @{$$prototype{'___args'}}     = ();
    @{$$prototype{'argnames'}}    = ();
    @{$$prototype{'___argnames'}} = ();
    @{$$prototype{'argtypes'}}    = ();
    
    my @args = split(/,/,$arguments);

    # Fix function pointer arguments and build $$prototype{'args'} 

    my $par_cnt = 0;
    foreach my $arg (@args) {
	# Strip whitespaces
	$arg =~ s/\s*(.*?)\s*/$1/;

	if ($par_cnt != 0) {
	    my $old_arg = pop @{$$prototype{'args'}};
	    
	    push @{$$prototype{'args'}}, $old_arg . "," . $arg;
	}
	else {
	    push @{$$prototype{'args'}}, $arg;
	}

	# Count parentheses (a function pointer arguments is processed
	# when $par_cnt is 0).
	$par_cnt += ( $arg =~ tr/\(/\(/ );
	$par_cnt -= ( $arg =~ tr/\)/\)/ );
    }

    $$prototype{'numargs'} = $#{$$prototype{'args'}} + 1;
    $$prototype{'numregs'} = $#{$$prototype{'regs'}} + 1;
    
    # varags -> stdarg (stdarg is a tag list) Example:
    # varargs: LONG Printf( STRPTR format, ... );
    # stdarg: BOOL AslRequestTags( APTR requester, Tag Tag1, ... );

    if ($$prototype{'type'} eq 'varargs' &&
	$varargs_type =~ /^\s*(const|CONST)?\s*struct\s+TagItem\s*\*\s*$/ ) {
	$$prototype{'type'} = 'stdarg';
    }
    
    # Make sure we have the same number of arguments as registers, or,
    # if this is a stdarg function, possible one extra, á la "Tag, ..."
    
    if ( !($$prototype{'numargs'} == $$prototype{'numregs'} || 
	   (($$prototype{'type'} eq 'stdarg' ||
	     $$prototype{'type'} eq 'varargs') &&
	    $$prototype{'numargs'} == $$prototype{'numregs'} + 1)) ) {

	print STDERR "Failed to parse arguments/registers on SFD " .
	    "line $$prototype{'line'}:\n$$prototype{'value'}\n";
	die;
    }

    my $type = '';
    
    foreach my $arg (@{$$prototype{'args'}}) {
	my $name    = '';
	my $___name = '';
	my $___arg  = '';

	if ($arg =~ /.*\(.*?\)\s*\(.*\)/) {
	    my $type1;
	    my $type2;
	    
	    ($type1, $name, $type2) =
		( $arg =~ /^\s*(.*)\(\s*\*\s*(\w+)\s*\)\s*(\(.*\))\s*/ );
	    $type = "$type1(*)$type2";
	    $___name = "___$name";
	    $___arg = "$type1(*___$name)$type2";
	}
	elsif ($arg !~ /^\.\.\.$/) {
	    ($type, $name) = ( $arg =~ /^\s*(.*?)\s+(\w+)\s*$/ );
	    $___name = "___$name";
	    $___arg = "$type ___$name";
	}
	else {
	    $type = $varargs_type;
	    $name = '...';
	    $___name = '...';
	    $___arg = '...';
	}

	if ($type eq '' || $name eq '' ) {
	    print STDERR "Type or name missing from '$arg'.\n";
	    die;
	}

	push @{$$prototype{'___args'}}, $___arg;
	push @{$$prototype{'argnames'}}, $name;
	push @{$$prototype{'___argnames'}}, $___name;

	push @{$$prototype{'argtypes'}}, $type;
    }
}



sub BEGIN {
    my $old_output = '';


### close_output: Close the output file if necessary  #########################

    sub close_output () {
	close (STDOUT);
	$old_output = '';
    }
    

### check_output: Check if the file will be reopended by open_output ##########

    sub will_close_output ( $$ ) {
	my $sfd      = shift;
	my $function = shift;

	my $new_output = $output;

	$new_output =~ s/%f/$function/;
	$new_output =~ s/%b/$$sfd{'base'}/;
	$new_output =~ s/%l/$$sfd{'libname'}/;
	$new_output =~ s/%n/$$sfd{'basename'}/;

	if( $old_output ne '' &&
	    $new_output ne $old_output ) {
	    return 1;
	}
	else {
	    return 0;
	}
    }
    
### open_output: (Re)open the output file if necessary  #######################

    sub open_output ( $$ ) {
	my $sfd      = shift;
	my $function = shift;

	my $new_output = $output;

	$new_output =~ s/%f/$function/;
	$new_output =~ s/%b/$$sfd{'base'}/;
	$new_output =~ s/%l/$$sfd{'libname'}/;
	$new_output =~ s/%n/$$sfd{'basename'}/;

	if( $new_output ne $old_output ) {

	    close_output ();

	    if ($new_output eq '-') {
		open (STDOUT, ">&OLDOUT") or die;
	    }
	    else {
		open (STDOUT, ">" . $new_output) or die;

		if (!$quiet) {
		    print STDERR "Writing to '$new_output'\n";
		}
	    }
	    
	    $old_output = $new_output;

	    return 1;
	}
	else {
	    return 0;
	}
    }
}
