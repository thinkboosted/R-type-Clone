#!/usr/bin/perl
use strict;
use warnings;

my $input_file = 'src/engine/modules/ECSManager/LuaECSManager/LuaECSManager.cpp';
my $output_file = 'definitions/game_api.lua';

my %type_map = (
    'std::string'   => 'string',
    'sol::table'    => 'table',
    'sol::object'   => 'any',
    'sol::function' => 'function',
    'int'           => 'integer',
    'size_t'        => 'integer',
    'bool'          => 'boolean',
    'float'         => 'number',
    'double'        => 'number',
    'Vec3'          => 'Vec3'
);

open(my $fh, '<', $input_file) or die "Cannot open $input_file: $!";
my @lines = <$fh>;
close($fh);

my @functions;
my $i = 0;
while ($i < @lines) {
    my $line = $lines[$i];

    if ($line =~ /ecs\.set_function\("([^"]+)",\s*\[/) {
        my $func_name = $1;

        # Extract parameters from lambda signature
        my $full_signature = $line;
        my $j = $i;

        # Collect lines until we find the closing bracket of the lambda parameters
        while ($j < @lines && $full_signature !~ /\]\s*\([^)]*\)\s*({|->)/) {
            $j++;
            if ($j < @lines) {
                $full_signature .= $lines[$j];
            }
        }

        my @params;
        if ($full_signature =~ /\]\s*\(([^)]*)\)/) {
            my $param_str = $1;
            my @param_parts = split(/,/, $param_str);

            foreach my $param (@param_parts) {
                $param =~ s/^\s+|\s+$//g; # trim
                next if $param eq 'this';

                if ($param =~ /const\s+(\w+(?:::\w+)*)\s*&?\s*(\w+)/) {
                    my $type = $1;
                    my $name = $2;
                    my $lua_type = $type_map{$type} // $type;
                    push @params, { name => $name, type => $lua_type };
                } elsif ($param =~ /(\w+(?:::\w+)*)\s+(\w+)/) {
                    my $type = $1;
                    my $name = $2;
                    my $lua_type = $type_map{$type} // $type;
                    push @params, { name => $name, type => $lua_type };
                }
            }
        }

        push @functions, {
            name => $func_name,
            params => \@params,
            line => $i + 1
        };

        $i = $j + 1;
    } else {
        $i++;
    }
}

# Generate Lua definition file
open(my $out, '>', $output_file) or die "Cannot write to $output_file: $!";

print $out "-- Auto-generated Lua API definitions\n";
print $out "-- Generated from LuaECSManager.cpp\n\n";

foreach my $func (@functions) {
    print $out "---\n";
    print $out "--- \@function " . $func->{name} . "\n";

    # Print parameters
    foreach my $param (@{$func->{params}}) {
        print $out "--- \@param " . $param->{name} . " " . $param->{type} . "\n";
    }

    print $out "function ecs." . $func->{name} . "(";
    my @param_names = map { $_->{name} } @{$func->{params}};
    print $out join(", ", @param_names);
    print $out ") end\n\n";
}

close($out);

print "Generated $output_file with " . scalar(@functions) . " functions.\n";
