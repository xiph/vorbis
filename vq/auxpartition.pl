#!/usr/bin/perl

my($auxparts,@partpoint)=@ARGV;

if($#partpoint<0){
    &usage;
}

while (<STDIN>) {    
    my@nums = ();
    @nums = split(/,/);
    my$cols=$#nums;
    my$j,$i=0;
    for(;$j<=$#partpoint;$j++){
	for(;$i<$partpoint[$j] && $i<$cols;$i++){
	    print $nums[$i]+($j*$auxparts).", ";
	}
    }
    for(;$i<$cols;$i++){
	print $nums[$i]+($j*$auxparts).", ";
    }
    print "\n";
}
    
sub usage{
    print "\nOggVorbis auxbook spectral partitioner\n\n";
    print "auxpartition.pl <entries> <partitionpoint> [<partitionpoint>...]\n\n";
    exit(1);
}
