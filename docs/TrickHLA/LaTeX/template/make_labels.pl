#####################################################################
# Creation:
#    Author: David Hammen
#    Date:   25 October 2005
#--------------------------------------------------------------------
# Modified:
#    Author: David Hammen and Susan Lim
#    Date:   April 2006
#    Description: Removed file:: from $labelinfo[4] assignment
#####################################################################

#!/usr/bin/perl
# Syntax: make_labels.pl MODELxxx[.suffix] MODELyyy[.suffix] ...

use strict;

sub make_labels($);
sub split_brackets($);
sub join_brackets(@);


foreach my $file (@ARGV) {
   my ($file_prefix);
   ($file_prefix = $file) =~ s/\..*//;
   if ($file_prefix =~ /(Reqt|IVV)$/) {
   } else {
      warn "Skipping $file (unknown type)\n";
      next;
   }
   make_labels $file_prefix;
}


################################################################################


sub make_labels($) {
   my ($file_prefix) = @_;

   open AUX, '<', "${file_prefix}.aux"
      or die "No such file ${file_prefix}.aux";
   open DEF, '>', "$file_prefix.def"
      or die "Can't create $file_prefix.def\n";
   while (<AUX>) {
      chomp;
      if (/^\\newlabel(.*)/) {
         my ($labelname, $labelinfo, @junk) = split_brackets $1;
         die unless defined $labelinfo && scalar @junk == 0;
         my @labelinfo = split_brackets $labelinfo;
         die unless scalar @labelinfo == 5;
         if ($labelinfo[3] =~ /^(sub)?(requirement|inspection|test)\./) {
            $labelinfo[4] = "${file_prefix}.pdf";
            print DEF
                  '\newlabel' .
                  join_brackets ($labelname, join_brackets @labelinfo) . "\n";
         }
      }
   }
   close AUX;
   close DEF;
}


sub split_brackets($) {
   my ($labelspec) = @_;
   my @labelargs = ();
   my $level = 0;
   my $curr = '';
   foreach my $char (split '', $labelspec) {
      if ($char eq '{') {
         $curr .= $char if ($level > 0);
         $level++;
      } elsif ($char eq '}') {
         $level--;
         if ($level == 0) {
            push @labelargs, $curr;
            $curr = '';
         } else {
            $curr .= $char;
         }
      } else {
         $curr .= $char;
      }
   }
   die "Garbage $curr" if $curr ne '';
   return @labelargs;
}

sub join_brackets(@) {
   '{' . join('}{', @_) . '}';
}
