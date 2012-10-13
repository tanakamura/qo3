#!/usr/bin/perl -w

use strict;

my $addr_table = "";
my $name = "";
my $off = 0;
my $entry2symbol_table = "";
my $name_table = "";
my $num_entry = 0;

while (<>) {
    chomp($_);
    my ($addr, $t, $name) = split(/ /, $_);

    $name_table = $name_table . $name . "\x00";
    $addr_table = $addr_table . pack("V*", hex($addr));
    $entry2symbol_table = $entry2symbol_table . pack("V*", $off);
    $off = $off + length($name) + 1;

    $num_entry++;
}

my $off_to_symbols = $num_entry * 8 + 8;

print(pack("V*V*", $num_entry, $off_to_symbols));
print($addr_table);
print($entry2symbol_table);
print($name_table);
