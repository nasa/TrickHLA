#!/usr/bin/perl

#####################################################################
# Purpose:
#    Determine path of first directory relative to second,
#    the latter of which defaults to the current working directory.
#    The second directory must be subsidiary to the first.
# Creation:
#    Author: David Hammen
#    Date:   July 2006
#####################################################################


use Cwd 'abs_path';

my $target = shift;
my $source = shift;
my $relpath;

$source = '.' unless (defined $source);

die "Usage: $1 <target> [<source>]\n"
  unless (defined $target) && (-d $target) && (-d $source);

$target = abs_path $target;
$source = abs_path $source;

if (($relpath = $source) =~ s|^$target/||) {
   $relpath =~ s|[^/][^/]*|..|g;
   print "$relpath\n";
} else {
   die "Can't find $source in $target\n";
}
