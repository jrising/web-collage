#!/usr/bin/perl

print "Content-type: text/html\n\n";

my $script = $ENV{'SCRIPT_NAME'};

# Which are we looking at?
my $master = $ENV{'QUERY_STRING'};
my $action = "$script?$master";

if ($master eq "" || !(-e "data/$master.dat")) {
    $master = "default";
    $action = "ping.pl";
}

my $command = "./web-collage $master ./bgcmd";
my $refresh = "<meta http-equiv=\"refresh\" content=\"1\" />";
my $pingbutton = "stop refresh";

if ($ENV{'REQUEST_METHOD'} eq "POST") {
    read(STDIN, $request, $ENV{'CONTENT_LENGTH'}) || die "Could not get data";

    @parameter_list = split(/&/, $request);
    foreach (@parameter_list) {
	($name, $value) = split(/=/);
	$name =~ s/\+/ /g; # replace "+" with  spaces
	$name =~ s/%([0-9A-F][0-9A-F])/pack("c",hex($1))/ge; # replace %nn with characters
	$value =~ s/\+/ /g; # repeat for the value ...
	$value =~ s/%([0-9A-F][0-9A-F])/pack("c",hex($1))/ge;
        $passed{$name} = $value;
    }

    if (defined($passed{'url'})) {
	$passed{'url'} =~ s/\"/\\\"/g; # protect quotes
	$command .= ' "' . $passed{'url'} . '"';
    }

    if (defined($passed{'submit'}) && $passed{'submit'} eq "stop refresh") {
	$refresh = "";
	$pingbutton = "auto-refresh";
    }
}

# Ping it!
`rm last.log`;
`nice $command > last.log`;

print "
<html>
  <head>
    $refresh
    <title>Tapestry Ping - $master</title>
  </head>
  <body>
    <center>
      <form action=\"$action\" method=\"post\">
	<input type=\"submit\" name=\"submit\" value=\"$pingbutton\" />
        <input type=\"text\" name=\"url\" size=\"60\" maxsize=\"120\" />
	<input type=\"submit\" name=\"submit\" value=\"add new url\" /><br />
        <img src=\"data/$master.jpg\" />
      </form>
    </center>
  </body>
</html>";
