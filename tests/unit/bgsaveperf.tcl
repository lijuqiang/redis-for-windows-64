start_server {tags {"other"}} {
    if {$::force_failure} {
        # This is used just for test suite development purposes.
        test {Failing test} {
            format err
        } {ok}
    }


    test {BGSAVE string copy on write latency} {
        waitForBgsave r
        r flushdb
        puts "Measuring BGSAVE for 1,000,000 strings"
        set iter1 1000000
        set step1 1
        set start [clock clicks -milliseconds]
        for {set i 0} {$i < $iter1} {incr i $step1} {
            r set $i abcdefghij
        }
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to create items                : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        r set 500 xyz
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to modify first value (no save): [expr double($elapsed)/1000]"
        waitForBgsave r
        set bgstart [clock clicks -milliseconds]
        r bgsave
        waitForBgsave r
        set elapsed [expr [clock clicks -milliseconds]-$bgstart]
        puts "time for RO bgsave to complete      : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        set bgstart [clock clicks -milliseconds]
        r bgsave
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to start bgsave                : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        r set 502 xyz
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to modify first value (saving) : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        r set 503 xyz
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to modify second value (saving): [expr double($elapsed)/1000]"
        waitForBgsave r
        set elapsed [expr [clock clicks -milliseconds]-$bgstart]
        puts "time for bgsave to complete         : [expr double($elapsed)/1000]"
        r flushdb
    } {OK}

    test {BGSAVE list copy on write latency} {
        waitForBgsave r
        r flushdb
        puts "Measuring BGSAVE for 1,000,000 strings in list"
        set iter1 1000000
        set step1 1
        set start [clock clicks -milliseconds]
        for {set i 0} {$i < $iter1} {incr i $step1} {
          r rpush mylist abcdefghij
        }
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to create items                : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        r rpush mylist abcdefghij
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to modify first value (no save): [expr double($elapsed)/1000]"
        waitForBgsave r
        set bgstart [clock clicks -milliseconds]
        r bgsave
        waitForBgsave r
        set elapsed [expr [clock clicks -milliseconds]-$bgstart]
        puts "time for RO bgsave to complete      : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        set bgstart [clock clicks -milliseconds]
        r bgsave
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to start bgsave                : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        r rpush mylist abcdefghij
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to modify first value (saving) : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        r rpush mylist abcdefghij
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to modify second value (saving): [expr double($elapsed)/1000]"
        waitForBgsave r
        set elapsed [expr [clock clicks -milliseconds]-$bgstart]
        puts "time for bgsave to complete         : [expr double($elapsed)/1000]"
        r flushdb
    } {OK}

    test {BGSAVE hash dictionary copy on write latency} {
        waitForBgsave r
        r flushdb
        puts "Measuring BGSAVE for 1,000,000 strings in hash dictionary"
        set iter1 1000000
        set step1 1
        set start [clock clicks -milliseconds]
        for {set i 0} {$i < $iter1} {incr i $step1} {
            r hset myhash $i abcdefghij
        }
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to create items                : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        r hset myhash 501 xyz
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to modify first value (no save): [expr double($elapsed)/1000]"
        waitForBgsave r
        set bgstart [clock clicks -milliseconds]
        r bgsave
        waitForBgsave r
        set elapsed [expr [clock clicks -milliseconds]-$bgstart]
        puts "time for RO bgsave to complete      : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        set bgstart [clock clicks -milliseconds]
        r bgsave
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to start bgsave                : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        r hset myhash 502 xyz
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to modify first value (saving) : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        r hset myhash 503 xyz
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to modify second value (saving): [expr double($elapsed)/1000]"
        waitForBgsave r
        set elapsed [expr [clock clicks -milliseconds]-$bgstart]
        puts "time for bgsave to complete         : [expr double($elapsed)/1000]"
        r flushdb
    } {OK}

    test {BGSAVE large set copy on write latency} {
        waitForBgsave r
        r flushdb
        puts "Measuring BGSAVE for 1,000,000 strings in set"
        set iter1 1000000
        set step1 1
        set start [clock clicks -milliseconds]
        for {set i 0} {$i < $iter1} {incr i $step1} {
            r sadd myset $i
        }
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to create items                : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        r sadd myset abc
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to modify first value (no save): [expr double($elapsed)/1000]"
        waitForBgsave r
        set bgstart [clock clicks -milliseconds]
        r bgsave
        waitForBgsave r
        set elapsed [expr [clock clicks -milliseconds]-$bgstart]
        puts "time for RO bgsave to complete      : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        set bgstart [clock clicks -milliseconds]
        r bgsave
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to start bgsave                : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        r sadd myset def
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to modify first value (saving) : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        r sadd myset xyz
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to modify second value (saving): [expr double($elapsed)/1000]"
        waitForBgsave r
        set elapsed [expr [clock clicks -milliseconds]-$bgstart]
        puts "time for bgsave to complete         : [expr double($elapsed)/1000]"
        r flushdb
    } {OK}

    test {BGSAVE large zset copy on write latency} {
        waitForBgsave r
        r flushdb
        puts "Measuring BGSAVE for 1,000,000 strings in ordered set"
        set iter1 1000000
        set step1 1
        set start [clock clicks -milliseconds]
        for {set i 0} {$i < $iter1} {incr i $step1} {
            r zadd myzset $i $i
        }
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to create items                : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        r zadd myzset 501 9999
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to modify first value (no save): [expr double($elapsed)/1000]"
        waitForBgsave r
        set bgstart [clock clicks -milliseconds]
        r bgsave
        waitForBgsave r
        set elapsed [expr [clock clicks -milliseconds]-$bgstart]
        puts "time for RO bgsave to complete      : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        set bgstart [clock clicks -milliseconds]
        r bgsave
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to start bgsave                : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        r zadd myzset 502 9998
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to modify first value (saving) : [expr double($elapsed)/1000]"
        set start [clock clicks -milliseconds]
        r zadd myzset 503 9997
        set elapsed [expr [clock clicks -milliseconds]-$start]
        puts "time to modify second value (saving): [expr double($elapsed)/1000]"
        waitForBgsave r
        set elapsed [expr [clock clicks -milliseconds]-$bgstart]
        puts "time for bgsave to complete         : [expr double($elapsed)/1000]"
        r flushdb
    } {OK}

}
