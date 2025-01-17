#!/usr/bin/env perl
#
# input list of files
# output docs in Markdown format to stdout
#

use strict;
use warnings;
use autodie;

foreach (@ARGV) {
  my $filename = $_;

  print "# $filename\n\n";

  my %symbols = ();
  open(my $file, $filename);
  while (<$file>) {
    if ($_ =~ m/GLOBAL\s+([a-zA-Z0-9_]+)/) {
      $symbols{$1} = "";
    }
  }
  close $file;

  my $comment = "";
  my $is_collecting_comment = 0;
  open($file, $filename);
  while (<$file>) {
    if ($_ =~ m/--\s*(.*)$/) {
      unless ($is_collecting_comment) {
        $comment = "";
        $is_collecting_comment = 1;
      }
      $comment .= "$1\n";
      unless ($is_collecting_comment) {
        $is_collecting_comment = 1;
      }
    } elsif ($_ =~ m/([a-zA-Z0-9_]+):/) {
      if (grep /$1/, keys %symbols) {
        if ($is_collecting_comment) {
          $symbols{$1} = $comment;
        }
      }

      $is_collecting_comment = 0;
    } else {
      $is_collecting_comment = 0;
    }
  }
  close $file;

  foreach (keys(%symbols)) {
    print "## $_\n\n";
    print $symbols{$_};
    print "\n";
  }
  print "---\n\n";
}


