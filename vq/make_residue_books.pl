#!/usr/bin/perl

# quick, very dirty little script so that we can put all the
# information for building a residue book set (except the original
# partitioning) in one spec file.

#eg:

# >res0_128_128
# haux res0_96_128aux.vqd 0,4,2
# :1 res0_128_128_1.vqd, 4, nonseq cull, 0 +- 1
# +a 4, nonseq, 0 +- .25 .5
# :2 res0_128_128_2.vqd, 4, nonseq, 0 +- 1 2
# :3 res0_128_128_3.vqd, 4, nonseq, 0 +- 1 3 5
# :4 res0_128_128_4.vqd, 2, nonseq, 0 +- 1 3 5 8 11
# :5 res0_128_128_5.vqd, 1, nonseq, 0 +- 1 3 5 8 11 14 17 20 24 28 31 35 39 
# +a 4, nonseq, 0 +- .5 1


die "Could not open $ARGV[0]: $!" unless open (F,$ARGV[0]);

while($line=<F>){

    print "\n#### $line\n\n";

    # >res0_128_128
    if($line=~m/^>(.*)/){
	# set the output name
	$globalname=$1;
	next;
    }

    # haux res0_96_128aux.vqd 0,4,2
    if($line=~m/^h(.*)/){
	# build a huffman book (no mapping) 
	my($name,$datafile,$arg)=split(' ',$1);
	my $command="huffbuild $datafile $arg > $globalname$name.vqh";
	print ">>> $command\n";
	die "Couldn't build huffbook.\n\tcommand:$command\n" 
	    if syst($command);
	next;
    }

    # :1 res0_128_128_1.vqd, 4, nonseq, 0 +- 1
    if($line=~m/^:(.*)/){
	my($namedata,$dim,$seqp,$vals)=split(',',$1);
	my($name,$datafile)=split(' ',$namedata);
	# build value list
	my$plusminus="+";
	my$list;
	my$count=0;
	foreach my$val (split(' ',$vals)){
	    if($val=~/\-?\+?\d+/){
		if($plusminus=~/-/){
		    $list.="-$val ";
		    $count++;
		}
		if($plusminus=~/\+/){
		    $list.="$val ";
		    $count++;
		}
	    }else{
		$plusminus=$val;
	    }
	}
	die "Couldn't open temp file temp$$.vql: $!" unless
	    open(G,">temp$$.vql");
	print G "$count $dim 0 ";
	if($seqp=~/non/){
	    print G "0\n$list\n";
	}else{	
	    print G "1\n$list\n";
	}
	close(G);

	my $command="latticebuild temp$$.vql > $globalname$name.vqh";
	print ">>> $command\n";
	die "Couldn't build latticebook.\n\tcommand:$command\n" 
	    if syst($command);

	my $command="latticehint $globalname$name.vqh > temp$$.vqh";
	print ">>> $command\n";
	die "Couldn't pre-hint latticebook.\n\tcommand:$command\n" 
	    if syst($command);
	
	if($seqp=~/cull/){
	    my $command="restune temp$$.vqh $datafile 1 > $globalname$name.vqh";
	    print ">>> $command\n";
	    die "Couldn't tune latticebook.\n\tcommand:$command\n" 
		if syst($command);
	}else{
	    my $command="restune temp$$.vqh $datafile > $globalname$name.vqh";
	    print ">>> $command\n";
	    die "Couldn't tune latticebook.\n\tcommand:$command\n" 
		if syst($command);
	}

	my $command="latticehint $globalname$name.vqh > temp$$.vqh";
	print ">>> $command\n";
	die "Couldn't post-hint latticebook.\n\tcommand:$command\n" 
	    if syst($command);

	my $command="mv temp$$.vqh $globalname$name.vqh";
	print ">>> $command\n";
	die "Couldn't rename latticebook.\n\tcommand:$command\n" 
	    if syst($command);

	my $command="rm temp$$.vql";
	print ">>> $command\n";
	die "Couldn't remove temp files.\n\tcommand:$command\n" 
	    if syst($command);

	next;
    }
}

sub syst{
    system(@_)/256;
}
