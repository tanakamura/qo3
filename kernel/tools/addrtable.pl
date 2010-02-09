#!/usr/bin/perl -w

use strict;

my $table = "";
my $name = "";
my $off = 0;
my $name_table = "";
my $num_entry = 0;

while (<>) {
    chomp($_);
    my ($addr, $t, $name) = split(/ /, $_);

    $name_table = $name_table . $name . "\x00";
    $table = $table . pack("V*V*", hex($addr), $off);
    $off = $off + length($name) + 1;

    $num_entry++;
}

print(pack("V*V*", 0, $num_entry));
print($table);
print($name_table);
