#!/usr/bin/env perl

use strict;
use warnings;
use autodie;

my $out_dir = "aleph_images";

`mkdir -p $out_dir`;

open( my $file, "aleph_images.roff" );
my $filename = "";
my $body     = -1;
my $collect  = 0;
my $i        = 0;
while (<$file>) {
  if ( $_ =~ /\.PS\s*\n/ ) {
    $body    = "";
    $collect = 1;
  }
  elsif ( $_ =~ /\.PE\s*\n/ ) {
    $filename =~ s/\n//;
    $filename =~ tr/A-Z /a-z_/;
    $filename =~ s/[^a-z0-9_]//g;

    # $filename = "$out_dir/$filename.png";

    printf "compiling $filename...\n";

    $body =~ s/"/\\"/g;
    $body =~ s/\n/\\n/g;
    $body =~ s/\$/\\\$/g;

    system "printf \".PS\n$body.PE\" | \
    groff -p -ms -Tps | \
    magick -colorspace LinearGray -density 100 - -trim -strip $out_dir/$filename.png";
    $i += 1;

    $collect = 0;
  }
  elsif ( $collect == 1 ) {
    $body .= "$_";
  }
  elsif ( $collect == 0 ) {
    $filename = $_;
  }
}
close $file;
