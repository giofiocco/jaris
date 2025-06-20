#!/usr/bin/env perl
#
# input list of files
# -r <filename> to record the output of the file
#
# files expected to be:
# command: ...
# exitcode: ...
# stdin:
# ...
# stdout:
# ...
# stderr:
# ...
#
# command is required
# stdin is optional
# exitcode, stdout, stderr are optionals and can be recorded
#

use strict;
use warnings;
use autodie;

my $tests = 0;
my @fails = ();
my $i     = 0;
while ( $i <= $#ARGV ) {
    $tests += 1;
    my $filename = $ARGV[$i];

    my $record = 0;
    if ( $filename eq "-r" ) {
        $record = 1;
        $i += 1;
        $filename = $ARGV[$i];
    }

    print "TEST $filename ... ";
    open( my $file, $filename );

    my $command  = "";
    my $exitcode = "";
    my $stdin    = "";
    my $stdout   = "";
    my $stderr   = "";

    while (<$file>) {
        $_ =~ s/^\s*//;
        $_ =~ s/\s*$//;
        if ( $_ eq "" ) {
        }
        elsif ( $_ =~ /^(command|exitcode|stdin|stdout|stderr):\s*(.+)\s*$/ ) {
            eval "\$$1 = \"$2\"";
        }
        elsif ( $_ =~ /^(command|exitcode|stdin|stdout|stderr):\s*$/ ) {
            my $field = $1;
            my $body  = "";
            my $line  = "";
            until ( eof($file) or ( $line = <$file> ) =~ /^__end$field\s*$/ ) {
                $body .= $line;
            }
            $body =~ s/"/\\"/g;
            eval "\$$field = \"$body\"";
        }
        else {
            print "\nERROR: unexpected '$_'\n";
            exit 1;
        }
    }
    close $file;

    if ( $command =~ /^\s*$/ ) {
        print "\nERROR: no command\n";
        exit 1;
    }

    my $escapedstdin = $stdin;
    $escapedstdin =~ s/\n/\\n/g;
    $escapedstdin =~ s/"/\\"/g;

    # print "\nRUNNING: printf \"$escapedstdin\" | $command\n";
    my $actualstdout   = `printf "$escapedstdin" | $command`;
    my $actualexitcode = $?;

    $stdout       =~ s/\n$//g;
    $actualstdout =~ s/\n$//g;

    if ($record) {
        open( my $file, ">", $filename );
        print $file "command: $command\n";
        unless ( $stdin =~ /^\s*$/ ) {
            print $file "stdin:\n";
            unless ( $stdin =~ /\n$/g ) { $stdin .= "\n"; }
            print $file $stdin;
            print $file "__endstdin\n";
        }
        print $file "exitcode: $actualexitcode\n";
        print $file "stdout:\n";
        unless ( $actualstdout =~ /\n$/g ) { $actualstdout .= "\n"; }
        print $file $actualstdout;
        print $file "\n__endstdout\n";
        close $file;
        print "RECORDED\n";
    }
    else {
        my $ok = 1;

        if ( $exitcode eq $actualexitcode ) {
            unless ( $stdout eq $actualstdout ) {
                $ok = 0;
                print "\nOUTPUTS DIFFER\n";
                open( my $file, ">", $filename . ".expected" );
                print $file $stdout;
                close $file;
                open( $file, ">", $filename . ".output" );
                print $file $actualstdout;
                close $file;
                system( "diff", "-y", "-d", "$filename.expected",
                    "$filename.output" );
                `rm $filename.expected $filename.output`;
            }
        }
        else {
            $ok = 0;
            print "\nEXPECTED EXIT CODE $exitcode, FOUND $actualexitcode\n";
        }
        if ($ok) {
            print "OK\n";
        }
        else {
            push( @fails, $filename );
        }
    }

    $i += 1;
}

print "TESTED $tests FILES, " . ( $#fails + 1 ) . " FAILED\n";
for (@fails) {
    print "FAILED: $_\n";
}
