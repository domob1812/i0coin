#!/usr/bin/php
<?
// configuration
$daemon = '/home/rsnel/src/i0coin/src/i0coind getpeerinfo';
$port = 7333;

$ret = `$daemon`;
if ($ret == '') { // we got no output, not even an empty array...
	echo("no response from $daemon\n");
	exit(1);
}

$data = json_decode($ret);
if ($data === NULL) {
	echo("error decoding json data from daemon\n");
	exit(1);
}

if (!is_array($data)) {
	echo("returned data from daemon is not an array!?!?!\n");
	exit(1);
}

$output = array();

// filter data
foreach ($data as $key => $host) {
	if ($host->banscore > 0) continue;
	if ($host->inbound) continue;
	
	if (preg_match("/^(\d+)\.(\d+)\.(\d+)\.(\d+):$port\$/", $host->addr, $m)) {
		// found possible IPV4 address
		if ($m[1] > 255) continue; // DAFUK?
		if ($m[2] > 255) continue;
		if ($m[3] > 255) continue;
		if ($m[4] > 255) continue;
		$output[] = $m[1].'.'.$m[2].'.'.$m[3].'.'.$m[4];
	}
}

if (!count($output)) {
	printf("no outbound connections found in $daemon\n");
	exit(1);
}

?>
;
; BIND data for i0seed.snel.it
;
$TTL	600
@	IN	SOA eniac.snel.it. hostmaster.snel.it. (
			<? echo (date("Ymdhis")); ?> ; Serial
			28800		; Refresh
			7200		; Retry
			2419200		; Expire
			86400 )		; Negative Cache TLL	
		NS	eniac.snel.it.
		NS	penta.snel.it.
<? foreach ($output as $ip) { ?>
		A	<? echo($ip."\n") ?>
<? } ?>
