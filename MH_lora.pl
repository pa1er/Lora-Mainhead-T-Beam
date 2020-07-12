#!/usr/bin/perl
# 
# Deze routine communiceert met de TTN, maakt meanhead, geeft dit terug en send een APRS bericht via Hamnet
#
# (C) 2019, Erik-Jan Roggekamp, PA1ER
# 
#
#  Bestandsnaam: Status.cgi
#  url opties:
#
#  Datum laatste aanpassing 13-09-2019
#

use DateTime;
#use strict;
#use warnings;
use CGI::Simple;
use JSON qw( decode_json );
use HTTP::Request;
use LWP::UserAgent;
use MIME::Base64;
use IO::Socket::INET;
use Geo::Coordinates::Converter;

sub latlong2maidenhead;

my $peerHost = "44.0.0.0";
my $peerPort = "14580";
my $message1 ="user A1AA-6 pass 12345 -1 vers PA1ERLoraHam 0.1\n";

my $filename = 'debuglog.txt';
my $dt = substr(DateTime->today(),0,10);
my $dtn = DateTime->now;
$dtt = $dtn->hms;
print "Content-Type: text/html\n\n";


open (my $fh, '>>', $filename) or die;

print $fh "-------- $dt - $dtt-------\n";
##########################################################################
#
# Alle gegevens via de uri uitlezen..
#
##########################################################################
@values = split(/&/,$ENV{'QUERY_STRING'});
foreach $i (@values) {
    ($querystr, $querydata) = split(/=/,$i);
    $queryinfo {"$querystr"} = "$querydata";

#print $fh "$querystr  = $querydata\n";

}

#################################################
#
#  Uitlezen van de header
#
#################################################
$file = 'header';
open(HEAD, "<$file");
@lines = <HEAD> ;
close(HEAD) ;
#print $fh "Lines:\n";
foreach $line (@lines){
#    print $fh  "****$line\n";
}

read(STDIN, $buffer, $ENV{'CONTENT_LENGTH'});


#print $fh "******  test json ******\n";

my $Json = decode_json($buffer);
my $devId = $Json->{'dev_id'};
my $portId = $Json->{'port'};
my $devCounter = $Json->{'counter'};
my $devTime    = $json->{'metadata'}{'time'};
my $Frequency  = $Json->{'metadata'}{'frequency'};
my $GTimeStamp = $Json->{'metadata'}{'gateways'}[0]{'time'};
my $Rssi       = $Json->{'metadata'}{'gateways'}[0]{'rssi'};  
my $latitude    = $Json->{'payload_fields'}{'latitude'};
my $longitude   = $Json->{'payload_fields'}{'longitude'};
my $HDOP        = $Json->{'payload_fields'}{'hdop'};
my $DownloadUrl = $Json->{'downlink_url'};

my $locator = latlong2maidenhead( $latitude, $longitude );

print $fh "$devId, $portId, $devCounter, $devTim, $Frequency,  $Rssi, $latitude, $longitude, =>  $locator, $HDOP\n $DownloadUrl\n\n";


my $payload = encode_base64($locator);
chomp $payload;
my $json1 = "{\"dev_id\": \"$devId\" ,\"port\": $portId,\"confirmed\": false,\"counter\":$devCounter,\"payload_raw\": \"$payload\"}";


my $req = HTTP::Request->new( 'PUT', $DownloadUrl );

$req->content( $json1 );
my $lwp = LWP::UserAgent->new;
my $res = $lwp->request( $req );

#print $fh " Locator sendback - \n$DownloadUrl \n $json1\n";
    if ($res->is_success) {
        print $fh  "$res->content";
        print $fh "OK\n";
    } else {
        print $fh $res->status_line . " NOK\n";
    }


#}


#close($fh);
print "200 OK\n\n";
 
##########################################################
#
#   Nu aprs er van maken en wegsturen als APRS
#
#########################################################

my $cnv = Geo::Coordinates::Converter->new( lat => $latitude, lng => $longitude, datum => 'wgs84');
my $cnv2 = $cnv->convert( wgs84 => 'dms' );
my $tmplat = $cnv2->lat;
my $tmplng = $cnv2->lng;
my @lat = split( '\.' ,  $tmplat);
my $aprsLat = sprintf("%02d%02d.%02d", int($lat[0]), int($lat[1]), int($lat[2]));
if (substr($latitude,0,1) eq "-") {
        $aprsLat .= "Z";
        }
else {
      	$aprsLat .= "N";
}
my @lng = split( '\.' ,  $tmplng);
my $aprsLng = sprintf("%03d%02d.%02d", int($lng[0]), int($lng[1]), int($lng[2]));
if (substr($longitude,0,1) eq "-") {
        $aprsLng .= "W";
        }
else {
      	$aprsLng .= "E";
}

my $message2 = sprintf("PA1ER-6>APRS,TCPIP*:=%s/%s(120/001Erik-Jan LoraHam\n", $aprsLat, $aprsLng);


#print "$message2  \n$aprsLat / $tmplat - $aprsLng\n";

my $socket = new IO::Socket::INET (
        PeerHost => $peerHost,
        PeerPort => $peerPort,
        Proto => 'tcp',
);
die "kan niet verbinden met server$!\n" unless $socket;
print " Verbonden met server\n";

my $size = $socket->send($message1);  
my $response = "";
$socket->recv($response, 10);  #Lees een paar caracters dat ik weet dat er wat aangekomen is
$size = $socket->send($message2);
$response = "";
$socket->recv($response, 10);
$socket->close();
print $fh "aprs: $message2\n-> $response\n";
#print $fh " APRS bericht verzonden\n";

close($fh);



exit;


sub latlong2maidenhead {
	# created by scruss/VA3PID on 02011/04/01
	#https://www.perlmonks.org/?node_id=912476
 
  # convert a WGS84 coordinate in decimal degrees
  #  to a Maidenhead grid location
  my ( $lat, $long ) = @_;
  my @divisors =
    ( 72000, 36000, 7200, 3600, 300, 150 );    # field size in seconds
  my @locator = ();
 
  # add false easting and northing, convert to seconds
  $lat  = ( $lat + 90 ) * 3600;
  $long = ( $long + 180 ) * 3600;
  for ( my $i = 0 ; $i < 3 ; $i++ ) {
    foreach ( $long, $lat ) {
      my $div  = shift(@divisors);
      my $part = int( $_ / $div );
      if ( $i == 1 ) {    # do the numeric thing for 2nd pair
        push @locator, $part;
      }
      else {              # character thing for 1st and 3rd pair
        push @locator,
          chr( ( ( $i < 1 ) ? ord('A') : ord('a') ) + $part );
      }
      $_ -= ( $part * $div );    # leaves remainder in $long or $lat
    }
  }
  return join( '', @locator );
}

