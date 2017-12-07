#!/usr/bin/env perl

use strict;
use warnings;

#my $cmake = "/home/pboettch/devel/upstream/cmake/build/bin/cmake";
my $cmake = "cmake";

my @variables;
my @commands;
my @properties;
my @modules;
my %keywords; # command => keyword-list

# unwanted upper-cases
my %unwanted = map { $_ => 1 } qw(VS CXX IDE NOTFOUND NO_ DFOO DBAR NEW);
	# cannot remove ALL - exists for add_custom_command

# control-statements
my %conditional = map { $_ => 1 } qw(if else elseif endif);
my %loop = map { $_ => 1 } qw(foreach while endforeach endwhile);

# decrecated
my %deprecated = map { $_ => 1 } qw(build_name exec_program export_library_dependencies install_files install_programs install_targets link_libraries make_directory output_required_files remove subdir_depends subdirs use_mangled_mesa utility_source variable_requires write_file);

# add some (popular) modules
push @modules, "ExternalProject";

# variables
open(CMAKE, "$cmake --help-variable-list|") or die "could not run cmake";
while (<CMAKE>) {
	chomp;
	next if /\</; # skip VARIABLES which contained <>-"templates"
	push @variables, $_;
}
close(CMAKE);

# transform all variables in a hash - to be able to use exists later on
my %variables = map { $_ => 1 } @variables;

# commands
open(CMAKE, "$cmake --help-command-list|");
while (my $cmd = <CMAKE>) {
	chomp $cmd;
	push @commands, $cmd;
}
close(CMAKE);

# now generate a keyword-list per command
foreach my $cmd (@commands) {
	my @word = extract_upper("$cmake --help-command $cmd|");

	next if scalar @word == 0;

	$keywords{$cmd} = [ sort keys %{ { map { $_ => 1 } @word } } ];
}

# and now for modules
foreach my $mod (@modules) {
	my @word = extract_upper("$cmake --help-module $mod|");

	next if scalar @word == 0;

	$keywords{$mod} = [ sort keys %{ { map { $_ => 1 } @word } } ];
}

# and now for generator-expressions
my @generator_expr = extract_upper("$cmake --help-manual cmake-generator-expressions |");

# properties
open(CMAKE, "$cmake --help-property-list|");
while (<CMAKE>) {
	chomp;
	push @properties, $_;
}
close(CMAKE);

# generate cmake.vim
open(IN,  "<cmake.vim.in") or die "could not read cmake.vim.in";
open(OUT, ">syntax/cmake.vim") or die "could not write to syntax/cmake.vim";

my @keyword_hi;

while(<IN>)
{
	if (m/\@([A-Z0-9_]+)\@/) { # match for @SOMETHING@
		if ($1 eq "COMMAND_LIST") {
			# do not include "special" commands in this list
			my @tmp = grep { ! exists $conditional{$_} and
			                 ! exists $loop{$_} and
			                 ! exists $deprecated{$_} } @commands;
			print OUT " " x 12 , "\\ ", join(" ", @tmp), "\n";
		} elsif ($1 eq "VARIABLE_LIST") {
			print OUT " " x 12 , "\\ ", join(" ", sort keys %variables), "\n";
		} elsif ($1 eq "MODULES") {
			print OUT " " x 12 , "\\ ", join("\n", @modules), "\n";
		} elsif ($1 eq "GENERATOR_EXPRESSIONS") {
			print OUT " " x 12 , "\\ ", join(" ", @generator_expr), "\n";
		} elsif ($1 eq "CONDITIONALS") {
			print OUT " " x 12 , "\\ ", join(" ", sort keys %conditional), "\n";
		} elsif ($1 eq "LOOPS") {
			print OUT " " x 12 , "\\ ", join(" ", sort keys %loop), "\n";
		} elsif ($1 eq "DEPRECATED") {
			print OUT " " x 12 , "\\ ", join(" ", sort keys %deprecated), "\n";
		} elsif ($1 eq "KEYWORDS") {
			foreach my $k (sort keys %keywords) {
				print OUT "syn keyword cmakeKW$k contained\n";
				print OUT " " x 12, "\\ ", join(" ", @{$keywords{$k}}), "\n";
				print OUT "\n";
				push @keyword_hi, "hi def link cmakeKW$k ModeMsg";
			}
		} elsif ($1 eq "KEYWORDS_HIGHLIGHT") {
			print OUT join("\n", @keyword_hi), "\n";
		} else {
			print "ERROR do not know how to replace $1\n";
		}
	} else {
		print OUT $_;
	}
}
close(IN);
close(OUT);

sub extract_upper
{
	my $input = shift;
	my @word;

	open(KW, $input);
	while (<KW>) {

		foreach my $w (m/\b([A-Z_]{2,})\b/g) {
			next
				if exists $variables{$w} or  # skip if it is a variable
				   exists $unwanted{$w} or   # skip if not wanted
				   grep(/$w/, @word);     # skip if already in array

			push @word, $w;
		}
	}
	close(KW);

	return @word;
}
