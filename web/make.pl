#!/usr/bin/perl

print "Content-type: text/html\n\n";

my $script = $ENV{'SCRIPT_NAME'};
my $message = "";

if ($ENV{'REQUEST_METHOD'} eq "POST") {
    read(STDIN, $request, $ENV{'CONTENT_LENGTH'}) || die "Could not get data";

    @parameter_list = split(/&/, $request);
    foreach (@parameter_list) {
	($name, $value) = split(/=/);
	$name =~ s/\+/ /g; # replace "+" with  spaces
	$name =~ s/%([0-9A-F][0-9A-F])/pack("c",hex($1))/ge; # replace %nn with characters
	$value =~ s/\+/ /g; # repeat for the value ...
	$value =~ s/%([0-9A-F][0-9A-F])/pack("c",hex($1))/ge;
	$value =~ s/\"/\\\"/g; # protect quotes
        $passed{$name} = $value;
    }

    my $master = $passed{'name'};
    my $initurl = $passed{'url'};
    my $width = $passed{'width'};
    my $height = $passed{'height'};

    if (!defined($master) || $master eq "") {
	$message = "No tapestry name provided.";
    } elsif (!defined($initurl) || $initurl eq "") {
	$message = "No initial url provided.";
    } elsif (!defined($width) || $width == 0 ||
	       !defined($height) || $height == 0 ||
	       $width * $height > 100000) {
	$message = "Invalid tapestry size.";
    } elsif (-e "data/$master.dat") {
	$message = "Tapestry <a href=\"ping.pl?$master\">$master</a> already exists.";
    } else {
	# Create new web collage
	`convert -size ${width}x${height} xc:transparent \"data/$master.jpg\"`;
	`./web-collage $master ./bgcmd \"$initurl\"`;
	$message = "Tapestry <a href=\"ping.pl?$master\">$master</a> created!  Ping it to fill it out.";
    }
} else {
    $message = "Fill in the form below to create a new tapestry.  Tapestries start empty, but grow with usage.";
}

print "
<html>
  <head>
    <title>Create a New Tapestry</title>
  <head>
  <body>
    <p>$message</p>

    <form action=\"$script\" method=\"post\">
      <table>
        <tr>
	  <td>Name:</td>
	  <td><input type=\"text\" name=\"name\" size=\"20\" /></td>
	</tr>
	<tr>
	  <td>Initial URL:</td>
	  <td><input type=\"text\" name=\"url\" size=\"60\" maxsize=\"120\" /></td>
	</tr>
	<tr>
	  <td>Size:</td>
	  <td><input type=\"text\" name=\"width\" size=\"4\" /> x <input type=\"text\" name=\"height\" size=\"4\" /> (may not exceed 100000 pixels)</td>
	</tr>
        <tr>
	  <td></td>
	  <td><input type=\"submit\" name=\"submit\" value=\"Create\" /></td>
	</tr>
      </table>
    </form>
  </body>
</html>";
    
